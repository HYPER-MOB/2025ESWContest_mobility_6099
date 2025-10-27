cmake_minimum_required(VERSION 3.10)
project(HYPERMOB_Platform_TCU VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3 /O2")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Library-CAN
)

# Source files (simplified - only REST client and main)
set(TCU_SOURCES
    src/tcu_main.c
    src/tcu_rest_client.c
)

# Library-CAN sources
set(LIBCAN_SOURCES
    Library-CAN/can_api.c
    Library-CAN/canmessage.c
    Library-CAN/channel.c
    Library-CAN/adapterfactory.c
    Library-CAN/adapter_linux.c
)

# Find required libraries
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)

# Find libsocketcan
pkg_check_modules(SOCKETCAN REQUIRED libsocketcan)

# Find cJSON
find_path(CJSON_INCLUDE_DIR cjson/cJSON.h)
find_library(CJSON_LIBRARY cjson)

if(NOT CJSON_INCLUDE_DIR OR NOT CJSON_LIBRARY)
    message(FATAL_ERROR "cJSON library not found. Install with: sudo apt-get install libcjson-dev")
endif()

# Executable
add_executable(hypermob-tcu
    ${TCU_SOURCES}
    ${LIBCAN_SOURCES}
)

# Link libraries
target_link_libraries(hypermob-tcu
    Threads::Threads
    CURL::libcurl
    ${SOCKETCAN_LIBRARIES}
    ${CJSON_LIBRARY}
    m
)

# Include directories for libraries
target_include_directories(hypermob-tcu PRIVATE
    ${CURL_INCLUDE_DIRS}
    ${SOCKETCAN_INCLUDE_DIRS}
    ${CJSON_INCLUDE_DIR}
)

# Installation
install(TARGETS hypermob-tcu
    RUNTIME DESTINATION bin
)

message(STATUS "")
message(STATUS "========================================")
message(STATUS " HYPERMOB Platform TCU (Simplified)")
message(STATUS "========================================")
message(STATUS " Sources: tcu_main.c + tcu_rest_client.c")
message(STATUS " Using Library-CAN directly")
message(STATUS "========================================")
