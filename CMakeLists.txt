cmake_minimum_required(VERSION 3.18)
project(SCA CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0)
pkg_check_modules(LIBNFC REQUIRED libnfc)

include_directories(
  ${GLIB_INCLUDE_DIRS}
  ${LIBNFC_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/ble
  ${CMAKE_CURRENT_SOURCE_DIR}/nfc
)

link_directories(
  ${GLIB_LIBRARY_DIRS}
  ${LIBNFC_LIBRARY_DIRS}
)

# CAN core (기존 소스 유지: 여기 파일들은 네 프로젝트에 실제로 존재해야 함)
add_library(can_core
  can_api.cpp
  canmessage.cpp
  adapter_factory.cpp
  adapter_debug.cpp
  linux_adapter.cpp
  channel.cpp
)
target_include_directories(can_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(can_core PUBLIC ${GLIB_LIBRARIES})

# BLE/NFC 모듈
add_library(sca_ble  ble/sca_ble_peripheral.cpp)
target_link_libraries(sca_ble PUBLIC ${GLIB_LIBRARIES})
target_include_directories(sca_ble PUBLIC ble)

add_library(sca_nfc  nfc/nfc_reader.cpp)
target_link_libraries(sca_nfc PUBLIC ${LIBNFC_LIBRARIES})
target_include_directories(sca_nfc PUBLIC nfc)

# 실행파일: ★ 파일 경로를 'src/'가 아니라 실제 위치로 수정
add_executable(rpi_can_router
  main.cpp
  sequencer.cpp
)

target_link_libraries(rpi_can_router
  PRIVATE can_core sca_ble sca_nfc
          ${GLIB_LIBRARIES} ${LIBNFC_LIBRARIES}
)

target_include_directories(rpi_can_router
  PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/ble
    ${CMAKE_SOURCE_DIR}/nfc
)
