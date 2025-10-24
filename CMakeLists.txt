cmake_minimum_required(VERSION 3.18)
project(SCA CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 필요 패키지
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0)
# libnfc
pkg_check_modules(LIBNFC REQUIRED libnfc)

include_directories(
  ${GLIB_INCLUDE_DIRS}
  ${LIBNFC_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/ble
  ${CMAKE_CURRENT_SOURCE_DIR}/nfc
)

link_directories(
  ${GLIB_LIBRARY_DIRS}
  ${LIBNFC_LIBRARY_DIRS}
)

# CAN core (기존 것을 그대로 추가)
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

# BLE/NFC 모듈을 정적 라이브러리로
add_library(sca_ble
  ble/sca_ble_peripheral.cpp
)
target_link_libraries(sca_ble PUBLIC ${GLIB_LIBRARIES})
target_include_directories(sca_ble PUBLIC ble)

add_library(sca_nfc
  nfc/nfc_reader.cpp
)
target_link_libraries(sca_nfc PUBLIC ${LIBNFC_LIBRARIES})

# 메인 실행파일
add_executable(rpi_can_router
  src/main.cpp
  src/sequencer.cpp
)
target_link_libraries(rpi_can_router
  PRIVATE can_core sca_ble sca_nfc
          ${GLIB_LIBRARIES} ${LIBNFC_LIBRARIES}
)
target_sources(rpi_can_router
    PRIVATE
        ${CMAKE_SOURCE_DIR}/nfc/nfc_adapter.cpp
        ${CMAKE_SOURCE_DIR}/ble/ble_adapter.cpp
        ${CMAKE_SOURCE_DIR}/src/sequencer.cpp
        ${CMAKE_SOURCE_DIR}/nfc/nfc_reader.cpp
        ${CMAKE_SOURCE_DIR}/ble/sca_ble_peripheral.cpp
)
# 인클루드 경로
target_include_directories(rpi_can_router
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/nfc
        ${CMAKE_SOURCE_DIR}/ble
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include  # can_api.hpp, canmessage.hpp 등
)
