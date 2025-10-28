cmake_minimum_required(VERSION 3.18)
project(SCA LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB   REQUIRED glib-2.0 gio-2.0)
pkg_check_modules(LIBNFC REQUIRED libnfc)

# ========== CAN core ==========
add_library(can_core
  can_api.cpp
  canmessage.cpp
  adapter_factory.cpp
  adapter_debug.cpp
  linux_adapter.cpp
  channel.cpp
)
target_include_directories(can_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${GLIB_INCLUDE_DIRS}
)
target_link_libraries(can_core PUBLIC ${GLIB_LIBRARIES})

# ========== BLE ==========
add_library(sca_ble
  ble/sca_ble_peripheral.cpp
)
target_include_directories(sca_ble PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/ble
  ${GLIB_INCLUDE_DIRS}
)
target_link_libraries(sca_ble PUBLIC ${GLIB_LIBRARIES})

# ========== NFC ==========
add_library(sca_nfc
  nfc/nfc_reader.cpp
)
target_include_directories(sca_nfc PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/nfc
  ${LIBNFC_INCLUDE_DIRS}
)
target_link_libraries(sca_nfc PUBLIC ${LIBNFC_LIBRARIES})

# ========== Camera ==========
add_library(sca_cam
  camera/camera_adapter.cpp
  camera/camera_runner.cpp
)
target_include_directories(sca_cam PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/camera
)
# �ʿ� �� ���⿡ libcamera/opencv �� �߰� ��ũ
# target_link_libraries(sca_cam PUBLIC libcamera ...) 

# ========== Executable ==========
add_executable(rpi_can_router
  src/main.cpp
  src/sequencer.cpp
)
target_include_directories(rpi_can_router PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/ble
  ${CMAKE_CURRENT_SOURCE_DIR}/nfc
  ${CMAKE_CURRENT_SOURCE_DIR}/camera
)
target_link_libraries(rpi_can_router PRIVATE
  can_core sca_ble sca_nfc sca_cam
  ${GLIB_LIBRARIES} ${LIBNFC_LIBRARIES}
)
