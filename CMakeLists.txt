cmake_minimum_required(VERSION 3.14)
project(sca_car CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

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
