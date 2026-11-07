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

#include "openauto/Projection/GStreamerVideoOutput.hpp"

namespace openauto
{
namespace projection
{

struct GStreamerVideoOutput::Private
{
    QPointer<QVideoSink> sink;
};

GStreamerVideoOutput::GStreamerVideoOutput(configuration::IConfiguration::Pointer configuration, QObject* parent)
    : QObject(parent)
    , VideoOutput(std::move(configuration))
    , d_(std::make_unique<Private>())
{
}

GStreamerVideoOutput::~GStreamerVideoOutput() = default;

bool GStreamerVideoOutput::open()
{
    return true;
}

bool GStreamerVideoOutput::init()
{
    return true;
}

void GStreamerVideoOutput::write(uint64_t, const aasdk::common::DataConstBuffer&)
{
}

void GStreamerVideoOutput::stop()
{
}

void GStreamerVideoOutput::setVideoSink(QVideoSink* sink)
{
    d_->sink = sink;
}

QVideoSink* GStreamerVideoOutput::videoSink() const
{
    return d_->sink;
}

void GStreamerVideoOutput::setContainerSize(const QSize&)
{
}

std::string GStreamerVideoOutput::decoderDescription() const
{
    return {};
}

std::string GStreamerVideoOutput::selectDecoder(const std::string&)
{
    return {};
}

}
}
