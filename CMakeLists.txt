cmake_minimum_required(VERSION 3.14)
project(ipc_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)   # Q_OBJECT 매크로 있는 클래스 자동 moc 처리
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Network)

add_executable(ipc_demo
    main.cpp
    ipc.cpp
    ipc.h
)

target_link_libraries(ipc_demo
    Qt5::Core
    Qt5::Network
)

