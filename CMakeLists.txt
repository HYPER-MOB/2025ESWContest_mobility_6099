cmake_minimum_required(VERSION 3.14)
project(ipc_demo LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Network)

add_executable(ipc_demo
    main.cpp
    ipc.cpp
    ipc.h
    # C 기반 CAN 관련 소스
    adapterfactory.c
    adapter_linux.c
    can_api.c
    canmessage.c
    channel.c
)

target_link_libraries(ipc_demo
    Qt5::Core
    Qt5::Network
    socketcan
    pthread
)

