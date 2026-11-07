cmake_minimum_required(VERSION 3.16...3.31)

project(openauto VERSION 2.1.0 LANGUAGES CXX)

option(OPENAUTO_TEST "Build the openauto unit tests" OFF)
option(OPENAUTO_AUTOAPP "Build the autoapp executable" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)

find_package(Threads REQUIRED)
find_package(Boost 1.71 REQUIRED CONFIG COMPONENTS system log)
find_package(Protobuf REQUIRED)
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Network Multimedia MultimediaWidgets Bluetooth)
find_package(aasdk 3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(RTAUDIO REQUIRED IMPORTED_TARGET rtaudio)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)

add_subdirectory(btservice_proto)
add_subdirectory(openauto)
if(OPENAUTO_AUTOAPP)
    add_subdirectory(autoapp)
endif()

if(OPENAUTO_TEST)
    enable_testing()
    add_subdirectory(tests)
endif()

set(openauto_cmake_dir ${CMAKE_INSTALL_LIBDIR}/cmake/openauto)
install(EXPORT openautoTargets
    NAMESPACE openauto::
    DESTINATION ${openauto_cmake_dir}
    COMPONENT headers)
configure_package_config_file(cmake/openautoConfig.cmake.in
    ${PROJECT_BINARY_DIR}/openautoConfig.cmake
    INSTALL_DESTINATION ${openauto_cmake_dir})
write_basic_package_version_file(${PROJECT_BINARY_DIR}/openautoConfigVersion.cmake
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${PROJECT_BINARY_DIR}/openautoConfig.cmake
    ${PROJECT_BINARY_DIR}/openautoConfigVersion.cmake
    DESTINATION ${openauto_cmake_dir}
    COMPONENT headers)

if(TARGET_ARCH)
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${TARGET_ARCH})
endif()
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "OpenDsh")
set(CPACK_PACKAGE_VENDOR "OpenDsh")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_COMPONENTS_ALL applications libraries headers Unspecified)
set(CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "Applications")
set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C++ Headers")
set(CPACK_COMPONENT_APPLICATIONS_DESCRIPTION "Applications provided by OpenAuto")
set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION "Shared libraries used to build programs with OpenAuto")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION "C/C++ header files and CMake package for use with OpenAuto")
set(CPACK_COMPONENT_LIBRARIES_GROUP "Development")
set(CPACK_COMPONENT_HEADERS_GROUP "Development")
set(CPACK_COMPONENT_GROUP_DEVELOPMENT_EXPANDED ON)
set(CPACK_COMPONENT_GROUP_DEVELOPMENT_DESCRIPTION "All of the tools you'll ever need to develop software")
set(CPACK_COMPONENT_HEADERS_DEPENDS libraries)
set(CPACK_COMPONENT_APPLICATIONS_DEPENDS libraries)
set(CPACK_ALL_INSTALL_TYPES Full Developer)
set(CPACK_INSTALL_TYPE_FULL_DISPLAY_NAME "Everything")
set(CPACK_COMPONENT_LIBRARIES_INSTALL_TYPES Developer Full)
set(CPACK_COMPONENT_HEADERS_INSTALL_TYPES Developer Full)
set(CPACK_COMPONENT_APPLICATIONS_INSTALL_TYPES Full)
include(CPack)
