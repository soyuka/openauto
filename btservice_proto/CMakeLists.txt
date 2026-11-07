file(GLOB proto_files CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.proto)

add_library(btservice_proto SHARED ${proto_files})
add_library(openauto::btservice_proto ALIAS btservice_proto)

protobuf_generate(TARGET btservice_proto LANGUAGE cpp
    IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(btservice_proto PUBLIC
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(btservice_proto PUBLIC protobuf::libprotobuf)

set_target_properties(btservice_proto PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

install(TARGETS btservice_proto
    EXPORT openautoTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/btservice_proto
    COMPONENT headers
    FILES_MATCHING PATTERN "*.pb.h"
    PATTERN CMakeFiles EXCLUDE)
