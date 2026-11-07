/*
*  This file is part of openauto project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  openauto is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  openauto is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with openauto. If not, see <http://www.gnu.org/licenses/>.
*/

#include <mutex>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <QAbstractVideoBuffer>
#include <QMargins>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include "OpenautoLog.hpp"
#include "openauto/Projection/GStreamerVideoOutput.hpp"

namespace openauto
{
namespace projection
{

namespace
{

struct DecoderCandidate
{
    const char* factory;
    const char* description;
};

// Slice threading keeps avdec_h264 at zero frames of decoder delay; frame threading would add one frame per thread.
constexpr DecoderCandidate cDecoderCandidates[] = {
    {"nvh264dec", "nvh264dec"},
    {"vah264dec", "vah264dec"},
    {"v4l2h264dec", "v4l2h264dec"},
    {"avdec_h264", "avdec_h264 thread-type=slice"},
    {"openh264dec", "openh264dec"},
};

void ensureGStreamer()
{
    if(!gst_is_initialized())
    {
        gst_init(nullptr, nullptr);
    }
}

bool factoryExists(const std::string& name)
{
    GstElementFactory* factory = gst_element_factory_find(name.c_str());
    if(factory == nullptr)
    {
        return false;
    }
    gst_object_unref(factory);
    return true;
}

std::string firstElementName(const std::string& description)
{
    const auto begin = description.find_first_not_of(" \t");
    if(begin == std::string::npos)
    {
        return {};
    }
    const auto end = description.find_first_of(" \t!", begin);
    return description.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
}

QVideoFrameFormat::PixelFormat toPixelFormat(GstVideoFormat format)
{
    switch(format)
    {
    case GST_VIDEO_FORMAT_NV12:
        return QVideoFrameFormat::Format_NV12;
    case GST_VIDEO_FORMAT_I420:
        return QVideoFrameFormat::Format_YUV420P;
    default:
        return QVideoFrameFormat::Format_Invalid;
    }
}

QVideoFrameFormat toFrameFormat(const GstVideoInfo& info)
{
    QVideoFrameFormat format(QSize(GST_VIDEO_INFO_WIDTH(&info), GST_VIDEO_INFO_HEIGHT(&info)), toPixelFormat(GST_VIDEO_INFO_FORMAT(&info)));

    switch(info.colorimetry.matrix)
    {
    case GST_VIDEO_COLOR_MATRIX_BT709:
        format.setColorSpace(QVideoFrameFormat::ColorSpace_BT709);
        break;
    case GST_VIDEO_COLOR_MATRIX_BT601:
        format.setColorSpace(QVideoFrameFormat::ColorSpace_BT601);
        break;
    default:
        break;
    }
    if(info.colorimetry.transfer == GST_VIDEO_TRANSFER_BT709)
    {
        format.setColorTransfer(QVideoFrameFormat::ColorTransfer_BT709);
    }
    format.setColorRange(info.colorimetry.range == GST_VIDEO_COLOR_RANGE_0_255 ? QVideoFrameFormat::ColorRange_Full : QVideoFrameFormat::ColorRange_Video);
    return format;
}

class GstSampleVideoBuffer final: public QAbstractVideoBuffer
{
public:
    GstSampleVideoBuffer(GstBuffer* buffer, const GstVideoInfo& info, QVideoFrameFormat format)
        : buffer_(gst_buffer_ref(buffer))
        , info_(info)
        , format_(std::move(format))
    {
    }

    ~GstSampleVideoBuffer() override
    {
        unmap();
        gst_buffer_unref(buffer_);
    }

    MapData map(QVideoFrame::MapMode mode) override
    {
        MapData data;
        // Decoder pool buffers are shared with GStreamer, so only read access is safe.
        if(mode != QVideoFrame::ReadOnly)
        {
            return data;
        }
        if(!mapped_)
        {
            if(!gst_video_frame_map(&frame_, &info_, buffer_, GST_MAP_READ))
            {
                return data;
            }
            mapped_ = true;
        }

        data.planeCount = static_cast<int>(GST_VIDEO_FRAME_N_PLANES(&frame_));
        for(int plane = 0; plane < data.planeCount && plane < 4; ++plane)
        {
            data.bytesPerLine[plane] = GST_VIDEO_FRAME_PLANE_STRIDE(&frame_, plane);
            data.data[plane] = static_cast<uchar*>(GST_VIDEO_FRAME_PLANE_DATA(&frame_, plane));
            data.dataSize[plane] = data.bytesPerLine[plane] * GST_VIDEO_FRAME_COMP_HEIGHT(&frame_, plane);
        }
        return data;
    }

    void unmap() override
    {
        if(mapped_)
        {
            gst_video_frame_unmap(&frame_);
            mapped_ = false;
        }
    }

    QVideoFrameFormat format() const override
    {
        return format_;
    }

private:
    GstBuffer* buffer_;
    GstVideoInfo info_;
    QVideoFrameFormat format_;
    GstVideoFrame frame_{};
    bool mapped_ = false;
};

}

struct GStreamerVideoOutput::Private
{
    explicit Private(GStreamerVideoOutput* owner)
        : q(owner)
    {
    }

    static GstFlowReturn onNewSample(GstAppSink* appSink, gpointer userData);
    static GstBusSyncReply onBusMessage(GstBus*, GstMessage* message, gpointer);
    void deliver(QVideoFrame frame);
    void applyCrop();
    void teardown();

    GStreamerVideoOutput* q;

    std::mutex pipelineMutex;
    GstElement* pipeline = nullptr;
    GstAppSrc* source = nullptr;
    GstElement* crop = nullptr;
    QMargins cropMargins;
    std::string decoder;

    QPointer<QVideoSink> sink;

    std::mutex frameMutex;
    QVideoFrame pendingFrame;
    bool deliveryQueued = false;
};

GstFlowReturn GStreamerVideoOutput::Private::onNewSample(GstAppSink* appSink, gpointer userData)
{
    auto* self = static_cast<Private*>(userData);
    GstSample* sample = gst_app_sink_pull_sample(appSink);
    if(sample == nullptr)
    {
        return GST_FLOW_OK;
    }

    GstVideoInfo info;
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstCaps* caps = gst_sample_get_caps(sample);
    if(buffer != nullptr && caps != nullptr && gst_video_info_from_caps(&info, caps))
    {
        auto format = toFrameFormat(info);
        if(format.pixelFormat() != QVideoFrameFormat::Format_Invalid)
        {
            self->deliver(QVideoFrame(std::make_unique<GstSampleVideoBuffer>(buffer, info, std::move(format))));
        }
    }

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

GstBusSyncReply GStreamerVideoOutput::Private::onBusMessage(GstBus*, GstMessage* message, gpointer)
{
    GError* error = nullptr;
    gchar* debug = nullptr;

    switch(GST_MESSAGE_TYPE(message))
    {
    case GST_MESSAGE_ERROR:
        gst_message_parse_error(message, &error, &debug);
        OPENAUTO_LOG(error) << "[GStreamerVideoOutput] " << GST_MESSAGE_SRC_NAME(message) << ": " << error->message
                            << " (" << (debug != nullptr ? debug : "") << ")";
        break;
    case GST_MESSAGE_WARNING:
        gst_message_parse_warning(message, &error, &debug);
        OPENAUTO_LOG(warning) << "[GStreamerVideoOutput] " << GST_MESSAGE_SRC_NAME(message) << ": " << error->message
                              << " (" << (debug != nullptr ? debug : "") << ")";
        break;
    default:
        break;
    }

    if(error != nullptr)
    {
        g_error_free(error);
    }
    g_free(debug);
    return GST_BUS_DROP;
}

void GStreamerVideoOutput::Private::deliver(QVideoFrame frame)
{
    {
        std::lock_guard<std::mutex> lock(frameMutex);
        pendingFrame = std::move(frame);
        if(deliveryQueued)
        {
            return;
        }
        deliveryQueued = true;
    }

    // Coalesce: if the GUI thread falls behind, only the newest frame is handed to the sink.
    QMetaObject::invokeMethod(q, [this]() {
        QVideoFrame next;
        {
            std::lock_guard<std::mutex> lock(frameMutex);
            next = std::move(pendingFrame);
            pendingFrame = QVideoFrame();
            deliveryQueued = false;
        }
        if(sink != nullptr && next.isValid())
        {
            sink->setVideoFrame(next);
        }
    }, Qt::QueuedConnection);
}

void GStreamerVideoOutput::Private::applyCrop()
{
    if(crop != nullptr)
    {
        g_object_set(crop,
                     "left", cropMargins.left(),
                     "top", cropMargins.top(),
                     "right", cropMargins.right(),
                     "bottom", cropMargins.bottom(),
                     nullptr);
    }
}

void GStreamerVideoOutput::Private::teardown()
{
    if(pipeline != nullptr)
    {
        gst_element_set_state(pipeline, GST_STATE_NULL);
    }
    if(source != nullptr)
    {
        gst_object_unref(source);
        source = nullptr;
    }
    if(crop != nullptr)
    {
        gst_object_unref(crop);
        crop = nullptr;
    }
    if(pipeline != nullptr)
    {
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
}

GStreamerVideoOutput::GStreamerVideoOutput(configuration::IConfiguration::Pointer configuration, QObject* parent)
    : QObject(parent)
    , VideoOutput(std::move(configuration))
    , d_(std::make_unique<Private>(this))
{
    ensureGStreamer();
}

GStreamerVideoOutput::~GStreamerVideoOutput()
{
    std::lock_guard<std::mutex> lock(d_->pipelineMutex);
    d_->teardown();
}

std::string GStreamerVideoOutput::selectDecoder(const std::string& configured)
{
    ensureGStreamer();

    const auto configuredElement = firstElementName(configured);
    if(!configuredElement.empty())
    {
        if(factoryExists(configuredElement))
        {
            return configured;
        }
        OPENAUTO_LOG(warning) << "[GStreamerVideoOutput] configured decoder " << configuredElement << " is not available, picking one automatically";
    }

    for(const auto& candidate : cDecoderCandidates)
    {
        if(factoryExists(candidate.factory))
        {
            return candidate.description;
        }
    }
    return {};
}

bool GStreamerVideoOutput::open()
{
    std::lock_guard<std::mutex> lock(d_->pipelineMutex);
    if(d_->pipeline != nullptr)
    {
        return true;
    }

    d_->decoder = selectDecoder(configuration_->getVideoDecoder());
    if(d_->decoder.empty())
    {
        OPENAUTO_LOG(error) << "[GStreamerVideoOutput] no H.264 decoder found, install gstreamer1.0-libav";
        return false;
    }
    OPENAUTO_LOG(info) << "[GStreamerVideoOutput] decoder: " << d_->decoder;

    const std::string description =
        "appsrc name=source is-live=true format=time do-timestamp=true stream-type=stream"
        " caps=video/x-h264,stream-format=byte-stream"
        " ! h264parse"
        " ! " + d_->decoder +
        " ! queue leaky=downstream max-size-buffers=1 max-size-bytes=0 max-size-time=0"
        " ! videoconvert"
        " ! videocrop name=crop"
        " ! appsink name=sink sync=false drop=true max-buffers=1 caps=video/x-raw,format=(string){NV12,I420}";

    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(description.c_str(), &error);
    if(error != nullptr)
    {
        OPENAUTO_LOG(error) << "[GStreamerVideoOutput] cannot build pipeline: " << error->message;
        g_error_free(error);
        if(pipeline != nullptr)
        {
            gst_object_unref(pipeline);
        }
        return false;
    }

    d_->pipeline = pipeline;
    d_->source = GST_APP_SRC(gst_bin_get_by_name(GST_BIN(pipeline), "source"));
    d_->crop = gst_bin_get_by_name(GST_BIN(pipeline), "crop");
    d_->applyCrop();

    GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    GstAppSinkCallbacks callbacks{};
    callbacks.new_sample = &Private::onNewSample;
    gst_app_sink_set_callbacks(GST_APP_SINK(sink), &callbacks, d_.get(), nullptr);
    gst_object_unref(sink);

    GstBus* bus = gst_element_get_bus(pipeline);
    gst_bus_set_sync_handler(bus, &Private::onBusMessage, nullptr, nullptr);
    gst_object_unref(bus);

    if(gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        OPENAUTO_LOG(error) << "[GStreamerVideoOutput] cannot start pipeline";
        d_->teardown();
        return false;
    }
    return true;
}

bool GStreamerVideoOutput::init()
{
    emit startPlayback();
    return true;
}

void GStreamerVideoOutput::write(uint64_t, const aasdk::common::DataConstBuffer& buffer)
{
    std::lock_guard<std::mutex> lock(d_->pipelineMutex);
    if(d_->source == nullptr || buffer.size == 0)
    {
        return;
    }

    const auto result = gst_app_src_push_buffer(d_->source, gst_buffer_new_memdup(buffer.cdata, buffer.size));
    if(result != GST_FLOW_OK)
    {
        OPENAUTO_LOG(warning) << "[GStreamerVideoOutput] push buffer returned " << gst_flow_get_name(result);
    }
}

void GStreamerVideoOutput::stop()
{
    {
        std::lock_guard<std::mutex> lock(d_->pipelineMutex);
        d_->teardown();
    }
    {
        std::lock_guard<std::mutex> lock(d_->frameMutex);
        d_->pendingFrame = QVideoFrame();
    }
    emit stopPlayback();
}

void GStreamerVideoOutput::setVideoSink(QVideoSink* sink)
{
    d_->sink = sink;
}

QVideoSink* GStreamerVideoOutput::videoSink() const
{
    return d_->sink;
}

void GStreamerVideoOutput::setContainerSize(const QSize& size)
{
    if(size.width() <= 0 || size.height() <= 0)
    {
        return;
    }

    QSize video;
    switch(getVideoResolution())
    {
    case aasdk::proto::enums::VideoResolution::_1080p:
        video = QSize(1920, 1080);
        break;
    case aasdk::proto::enums::VideoResolution::_720p:
        video = QSize(1280, 720);
        break;
    default:
        video = QSize(800, 480);
        break;
    }

    const double widthRatio = static_cast<double>(size.width()) / video.width();
    const double heightRatio = static_cast<double>(size.height()) / video.height();
    int marginWidth = 0;
    int marginHeight = 0;
    if(widthRatio > heightRatio)
    {
        marginHeight = static_cast<int>((widthRatio * video.height() - size.height()) / widthRatio / 2);
    }
    else
    {
        marginWidth = static_cast<int>((heightRatio * video.width() - size.width()) / heightRatio / 2);
    }

    OPENAUTO_LOG(info) << "[GStreamerVideoOutput] container " << size.width() << "x" << size.height()
                       << ", cropping " << marginWidth << "x" << marginHeight << " per side";
    configuration_->setVideoMargins(QRect(0, 0, marginWidth * 2, marginHeight * 2));

    std::lock_guard<std::mutex> lock(d_->pipelineMutex);
    d_->cropMargins = QMargins(marginWidth, marginHeight, marginWidth, marginHeight);
    d_->applyCrop();
}

std::string GStreamerVideoOutput::decoderDescription() const
{
    std::lock_guard<std::mutex> lock(d_->pipelineMutex);
    return d_->decoder;
}

}
}
