# CMakeLists.txt (핵심 부분만)
cmake_minimum_required(VERSION 3.16)
project(SCA-Core CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

add_library(can_core
  can_api.cpp
  canmessage.cpp
  linux_adapter.cpp
  adapter_factory.cpp
  channel.cpp          # ← 반드시 포함
  adapter_debug.cpp    # ← 아래 예제 제공. 실제 사용 환경에 맞춰 교체 가능
)
# 서브모듈 빌드: BLE 실행파일, NFC/Camera 라이브러리
add_subdirectory(ble)
add_subdirectory(nfc)
add_subdirectory(camera)

add_executable(router
  src/main.cpp
  src/workers.cpp
  src/sequencer.cpp
)

target_include_directories(router PRIVATE include config)
target_link_libraries(router PRIVATE pthread nfclib camlib)
