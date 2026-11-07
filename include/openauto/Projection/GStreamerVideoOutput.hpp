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

#pragma once

#include <memory>
#include <string>
#include <QObject>
#include <QPointer>
#include <QSize>
#include <QVideoSink>
#include "openauto/Projection/VideoOutput.hpp"

namespace openauto
{
namespace projection
{

class GStreamerVideoOutput: public QObject, public VideoOutput
{
    Q_OBJECT

public:
    explicit GStreamerVideoOutput(configuration::IConfiguration::Pointer configuration, QObject* parent = nullptr);
    ~GStreamerVideoOutput() override;

    bool open() override;
    bool init() override;
    void write(uint64_t timestamp, const aasdk::common::DataConstBuffer& buffer) override;
    void stop() override;

    void setVideoSink(QVideoSink* sink);
    QVideoSink* videoSink() const;
    void setContainerSize(const QSize& size);
    std::string decoderDescription() const;

    static std::string selectDecoder(const std::string& configured);

signals:
    void startPlayback();
    void stopPlayback();

private:
    struct Private;
    std::unique_ptr<Private> d_;
};

}
}
