add_library(openauto SHARED
    App.cpp
    Configuration/Configuration.cpp
    Configuration/RecentAddressesList.cpp
    Projection/DummyBluetoothDevice.cpp
    Projection/GStreamerVideoOutput.cpp
    Projection/InputDevice.cpp
    Projection/LocalBluetoothDevice.cpp
    Projection/QtAudioInput.cpp
    Projection/QtAudioOutput.cpp
    Projection/RemoteBluetoothDevice.cpp
    Projection/RtAudioOutput.cpp
    Projection/SequentialBuffer.cpp
    Projection/VideoOutput.cpp
    Service/AndroidAutoEntity.cpp
    Service/AndroidAutoEntityFactory.cpp
    Service/AudioInputService.cpp
    Service/AudioService.cpp
    Service/BluetoothService.cpp
    Service/InputService.cpp
    Service/MediaAudioService.cpp
    Service/MediaStatusService.cpp
    Service/NavigationStatusService.cpp
    Service/Pinger.cpp
    Service/SensorService.cpp
    Service/ServiceFactory.cpp
    Service/SpeechAudioService.cpp
    Service/SystemAudioService.cpp
    Service/VideoService.cpp
    ${PROJECT_SOURCE_DIR}/btservice/AndroidBluetoothServer.cpp
    ${PROJECT_SOURCE_DIR}/btservice/AndroidBluetoothService.cpp
    ${PROJECT_SOURCE_DIR}/btservice/btservice.cpp
)
add_library(openauto::openauto ALIAS openauto)

file(GLOB_RECURSE openauto_public_headers CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/include/openauto/*.hpp
    ${PROJECT_SOURCE_DIR}/include/btservice/*.hpp)
target_sources(openauto PRIVATE ${openauto_public_headers} ${PROJECT_SOURCE_DIR}/include/OpenautoLog.hpp)

target_include_directories(openauto PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_options(openauto PRIVATE -Wall -pedantic)

target_link_libraries(openauto
    PUBLIC
        aasdk::aasdk
        openauto::btservice_proto
        Boost::system
        Boost::log
        Threads::Threads
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Multimedia
        Qt6::MultimediaWidgets
        Qt6::Bluetooth
        PkgConfig::RTAUDIO
    PRIVATE
        PkgConfig::GSTREAMER)

set_target_properties(openauto PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

install(TARGETS openauto
    EXPORT openautoTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/openauto ${PROJECT_SOURCE_DIR}/include/btservice
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT headers)
install(FILES ${PROJECT_SOURCE_DIR}/include/OpenautoLog.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT headers)
