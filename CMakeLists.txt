cmake_minimum_required(VERSION 3.5)

project(DCU_UI VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    AuthView/authwindow.cpp
    AuthView/authwindow.h
    AuthView/authwindow.ui

    DataCheckView/datacheckwindow.cpp
    DataCheckView/datacheckwindow.h
    DataCheckView/datacheckwindow.ui

    MainView/mainview.cpp
    MainView/mainview.h
    MainView/mainview.ui

    ipc_client.cpp
    ipc_client.h

    resources.qrc
)

add_executable(DCU_UI
    ${PROJECT_SOURCES}
)

target_link_libraries(DCU_UI PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
)

set_target_properties(DCU_UI PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(DCU_UI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/AuthView
    ${CMAKE_CURRENT_SOURCE_DIR}/DataCheckView
    ${CMAKE_CURRENT_SOURCE_DIR}/MainView
)

