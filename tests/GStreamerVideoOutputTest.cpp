#include <memory>
#include <vector>
#include <QFile>
#include <QVideoFrame>
#include <QVideoSink>
#include <QtTest>
#include "openauto/Configuration/Configuration.hpp"
#include "openauto/Projection/GStreamerVideoOutput.hpp"

using openauto::configuration::Configuration;
using openauto::projection::GStreamerVideoOutput;

namespace
{

std::vector<QByteArray> splitNalUnits(const QByteArray& stream)
{
    std::vector<qsizetype> starts;
    for(qsizetype i = 0; i + 3 <= stream.size(); ++i)
    {
        if(stream[i] == 0 && stream[i + 1] == 0 && stream[i + 2] == 1)
        {
            starts.push_back(i > 0 && stream[i - 1] == 0 ? i - 1 : i);
            i += 2;
        }
    }

    std::vector<QByteArray> units;
    for(size_t n = 0; n < starts.size(); ++n)
    {
        const qsizetype end = n + 1 < starts.size() ? starts[n + 1] : stream.size();
        units.push_back(stream.mid(starts[n], end - starts[n]));
    }
    return units;
}

struct FrameRecorder
{
    explicit FrameRecorder(QVideoSink& sink)
    {
        QObject::connect(&sink, &QVideoSink::videoFrameChanged, &sink, [this](const QVideoFrame& frame) {
            QVideoFrame copy(frame);
            if(!copy.isValid())
            {
                return;
            }
            sizes.push_back(copy.size());
            if(copy.map(QVideoFrame::ReadOnly))
            {
                mapped.push_back(copy.planeCount() > 0 && copy.bits(0) != nullptr && copy.mappedBytes(0) > 0);
                copy.unmap();
            }
            else
            {
                mapped.push_back(false);
            }
        });
    }

    std::vector<QSize> sizes;
    std::vector<bool> mapped;
};

}

class GStreamerVideoOutputTest: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void decodesByteStreamIntoVideoSink();
    void decodesAgainAfterStopAndReopen();
    void cropsToContainerAspectRatio();
    void autoSelectsAnAvailableDecoder();
    void fallsBackToAutoWhenConfiguredDecoderIsMissing();

private:
    void feed(GStreamerVideoOutput& output);

    std::vector<QByteArray> nalUnits_;
};

void GStreamerVideoOutputTest::initTestCase()
{
    QFile fixture(QStringLiteral(OPENAUTO_TEST_FIXTURES "/ball-800x480-30f.h264"));
    QVERIFY(fixture.open(QIODevice::ReadOnly));
    nalUnits_ = splitNalUnits(fixture.readAll());
    QVERIFY(nalUnits_.size() >= 30);
}

void GStreamerVideoOutputTest::feed(GStreamerVideoOutput& output)
{
    for(const auto& unit : nalUnits_)
    {
        output.write(0, aasdk::common::DataConstBuffer(unit.constData(), static_cast<size_t>(unit.size())));
    }
}

void GStreamerVideoOutputTest::decodesByteStreamIntoVideoSink()
{
    auto configuration = std::make_shared<Configuration>();
    configuration->setVideoResolution(aasdk::proto::enums::VideoResolution::_480p);
    QVideoSink sink;
    FrameRecorder recorder(sink);
    GStreamerVideoOutput output(configuration);
    output.setVideoSink(&sink);
    QSignalSpy started(&output, &GStreamerVideoOutput::startPlayback);
    QSignalSpy stopped(&output, &GStreamerVideoOutput::stopPlayback);

    QVERIFY(output.open());
    QVERIFY(output.init());
    feed(output);

    QTRY_VERIFY_WITH_TIMEOUT(!recorder.sizes.empty(), 10000);
    QCOMPARE(recorder.sizes.front(), QSize(800, 480));
    QVERIFY(recorder.mapped.front());
    QCOMPARE(started.count(), 1);

    output.stop();
    QTRY_COMPARE(stopped.count(), 1);
}

void GStreamerVideoOutputTest::decodesAgainAfterStopAndReopen()
{
    auto configuration = std::make_shared<Configuration>();
    QVideoSink sink;
    FrameRecorder recorder(sink);
    GStreamerVideoOutput output(configuration);
    output.setVideoSink(&sink);

    QVERIFY(output.open());
    feed(output);
    QTRY_VERIFY_WITH_TIMEOUT(!recorder.sizes.empty(), 10000);
    output.stop();

    QTest::qWait(100);
    recorder.sizes.clear();

    QVERIFY(output.open());
    feed(output);
    QTRY_VERIFY_WITH_TIMEOUT(!recorder.sizes.empty(), 10000);
    QCOMPARE(recorder.sizes.back(), QSize(800, 480));
    output.stop();
}

void GStreamerVideoOutputTest::cropsToContainerAspectRatio()
{
    auto configuration = std::make_shared<Configuration>();
    configuration->setVideoResolution(aasdk::proto::enums::VideoResolution::_480p);
    QVideoSink sink;
    FrameRecorder recorder(sink);
    GStreamerVideoOutput output(configuration);
    output.setVideoSink(&sink);
    output.setContainerSize(QSize(800, 400));

    QCOMPARE(configuration->getVideoMargins(), QRect(0, 0, 0, 80));

    QVERIFY(output.open());
    feed(output);

    QTRY_VERIFY_WITH_TIMEOUT(!recorder.sizes.empty(), 10000);
    QCOMPARE(recorder.sizes.back(), QSize(800, 400));
    output.stop();
}

void GStreamerVideoOutputTest::autoSelectsAnAvailableDecoder()
{
    const auto decoder = GStreamerVideoOutput::selectDecoder("");
    QVERIFY2(!decoder.empty(), "no H.264 decoder found");
    QVERIFY2(decoder.rfind("avdec_h264", 0) == 0, decoder.c_str());
}

void GStreamerVideoOutputTest::fallsBackToAutoWhenConfiguredDecoderIsMissing()
{
    QCOMPARE(GStreamerVideoOutput::selectDecoder("nosuchh264dec some-prop=1"), GStreamerVideoOutput::selectDecoder(""));
    QCOMPARE(GStreamerVideoOutput::selectDecoder("avdec_h264 max-threads=2"), std::string("avdec_h264 max-threads=2"));
}

QTEST_MAIN(GStreamerVideoOutputTest)
#include "GStreamerVideoOutputTest.moc"
