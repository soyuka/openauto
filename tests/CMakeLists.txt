find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(openauto_video_test GStreamerVideoOutputTest.cpp)
target_compile_definitions(openauto_video_test PRIVATE
    OPENAUTO_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
target_link_libraries(openauto_video_test PRIVATE openauto::openauto Qt6::Test)

add_test(NAME openauto_video_test COMMAND openauto_video_test)
set_tests_properties(openauto_video_test PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 120)

add_executable(openauto_service_factory_test ServiceFactoryTest.cpp)
target_link_libraries(openauto_service_factory_test PRIVATE openauto::openauto Qt6::Test)

add_test(NAME openauto_service_factory_test COMMAND openauto_service_factory_test)
set_tests_properties(openauto_service_factory_test PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 60)
