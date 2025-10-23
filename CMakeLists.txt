cmake_minimum_required(VERSION 3.10)
project(SCA_Tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wno-unused-function -Wno-missing-field-initializers)

# ===== 의존성 탐지 =====
find_package(PkgConfig REQUIRED)
pkg_check_modules(BLUEZ REQUIRED bluez)
pkg_check_modules(LIBNFC REQUIRED libnfc)
pkg_check_modules(GLIB  REQUIRED glib-2.0)
pkg_check_modules(GIO   REQUIRED gio-2.0) # 또는 gio-2.0>=2.56

# 전역으로 BlueZ/NFC include & lib 디렉터리
include_directories(${BLUEZ_INCLUDE_DIRS} ${LIBNFC_INCLUDE_DIRS})
link_directories(${BLUEZ_LIBRARY_DIRS} ${LIBNFC_LIBRARY_DIRS})

# ===== 실행파일들 =====
add_executable(test_ble_log tests/test_ble_log.cpp)
target_link_libraries(test_ble_log ${BLUEZ_LIBRARIES})

add_executable(test_nfc_log tests/test_nfc_log.cpp)
target_link_libraries(test_nfc_log ${LIBNFC_LIBRARIES})

add_executable(rpi_can_router tests/rpi_can_router.cpp)
target_link_libraries(rpi_can_router ${BLUEZ_LIBRARIES} ${LIBNFC_LIBRARIES})

# ---- sca_ble_peripheral (GIO/GLib 필요) ----
add_executable(sca_ble_peripheral src/sca_ble_peripheral.cpp)

target_include_directories(sca_ble_peripheral PRIVATE
  ${GLIB_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}
)

target_link_directories(sca_ble_peripheral PRIVATE
  ${GLIB_LIBRARY_DIRS}
  ${GIO_LIBRARY_DIRS}
)

target_compile_options(sca_ble_peripheral PRIVATE
  ${GLIB_CFLAGS_OTHER}
  ${GIO_CFLAGS_OTHER}
)

# 링크 순서: gio 먼저, 그다음 glib 계열
target_link_libraries(sca_ble_peripheral
  ${GIO_LIBRARIES}
  ${GLIB_LIBRARIES}
)
