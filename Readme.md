
# OpenAuto

### Community
[![Join the chat at https://gitter.im/publiclab/publiclab](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/openauto_androidauto/Lobby)

### Description
OpenAuto is an AndroidAuto(tm) headunit emulator based on aasdk library and Qt libraries. Main goal is to run this application on the RaspberryPI 3 board computer smoothly.

[See demo video](https://www.youtube.com/watch?v=k9tKRqIkQs8)

### Build Guide

Requires Qt 6.8, GStreamer 1.x, RtAudio 6 and an installed [aasdk](https://github.com/openDsh/aasdk) (its CMake package is found through `CMAKE_PREFIX_PATH`). On Debian 13 (trixie):

```
sudo apt-get install build-essential cmake ninja-build pkgconf \
    libboost-system-dev libboost-log-dev libprotobuf-dev protobuf-compiler \
    qt6-base-dev qt6-multimedia-dev qt6-connectivity-dev librtaudio-dev \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/aasdk/prefix
cmake --build build
cmake --install build
```

`-DOPENAUTO_TEST=ON` builds the tests (`ctest --test-dir build`). Downstream projects use `find_package(openauto 2)` and link `openauto::openauto`.

At runtime the video path needs `gstreamer1.0-plugins-base`, `gstreamer1.0-plugins-good`, `gstreamer1.0-plugins-bad` (h264parse) and an H.264 decoder, `gstreamer1.0-libav` (avdec_h264) on the Raspberry Pi 5, which has no hardware H.264 decoder. `Video.Decoder` in `openauto.ini` overrides the decoder with a gst-launch fragment, e.g. `avdec_h264 thread-type=frame` for 1080p throughput at the cost of latency.

### Supported functionalities
 - 480p, 720p and 1080p with 30 or 60 FPS
 - Video decoding through GStreamer (software or any available hardware H.264 decoder)
 - Audio playback from all audio channels (Media, System and Speech)
 - Audio input for voice commands
 - Touchscreen and buttons input
 - Bluetooth
 - Automatic launch after device hotplug
 - Automatic detection of connected Android devices
 - Wireless (WiFi) mode via head unit server (must be enabled in hidden developer settings)
 - User-friendly settings

### Supported platforms

 - Linux (X11 or Wayland), including the Raspberry Pi 5

### License
GNU GPLv3

Copyrights (c) 2018 f1x.studio (Michal Szwaj)

*AndroidAuto is registered trademark of Google Inc.*

### Used software
 - [aasdk](https://github.com/f1xpl/aasdk)
 - [Boost libraries](http://www.boost.org/)
 - [Qt libraries](https://www.qt.io/)
 - [CMake](https://cmake.org/)
 - [RtAudio](https://www.music.mcgill.ca/~gary/rtaudio/playback.html)
 - [GStreamer](https://gstreamer.freedesktop.org/)

### Remarks
**This software is not certified by Google Inc. It is created for R&D purposes and may not work as expected by the original authors. Do not use while driving. You use this software at your own risk.**