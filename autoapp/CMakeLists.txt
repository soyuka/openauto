add_executable(autoapp
    autoapp.cpp
    UI/ConnectDialog.cpp
    UI/connectdialog.ui
    UI/MainWindow.cpp
    UI/mainwindow.ui
    UI/SettingsWindow.cpp
    UI/settingswindow.ui
    assets/resources.qrc
    ${PROJECT_SOURCE_DIR}/include/autoapp/UI/ConnectDialog.hpp
    ${PROJECT_SOURCE_DIR}/include/autoapp/UI/MainWindow.hpp
    ${PROJECT_SOURCE_DIR}/include/autoapp/UI/SettingsWindow.hpp
)

set_target_properties(autoapp PROPERTIES AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/UI)
target_include_directories(autoapp PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_options(autoapp PRIVATE -Wall -pedantic)
target_link_libraries(autoapp PRIVATE openauto::openauto)

install(TARGETS autoapp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT applications)
