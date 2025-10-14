cmake_minimum_required(VERSION 3.10)
project(sca_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic)
include_directories(include)

find_package(Threads REQUIRED)
find_library(BLUETOOTH_LIB bluetooth)

add_executable(sca_core
  src/main.cpp
  src/common.cpp
  src/event_bus.cpp
  src/ble_module.cpp
  src/nfc_module.cpp
  src/can_iface.cpp
  src/orchestrator.cpp
  src/log_sink.cpp
  src/can_db.cpp
)

target_link_libraries(sca_core PRIVATE ${BLUETOOTH_LIB} Threads::Threads)
