cmake_minimum_required(VERSION 3.10)
project(SCA_Tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wno-unused-function -Wno-missing-field-initializers)

# ===== 의존성 탐지 =====
find_package(PkgConfig REQUIRED)
pkg_check_modules(BLUEZ REQUIRED bluez)
pkg_check_modules(LIBNFC REQUIRED libnfc)

include_directories(${BLUEZ_INCLUDE_DIRS} ${LIBNFC_INCLUDE_DIRS})
link_directories(${BLUEZ_LIBRARY_DIRS} ${LIBNFC_LIBRARY_DIRS})

# ===== 테스트 실행파일들 =====

# 1) BLE 스캔 테스트
add_executable(test_ble_log tests/test_ble_log.cpp)
target_link_libraries(test_ble_log ${BLUEZ_LIBRARIES})

# 2) NFC 폴링 테스트
add_executable(test_nfc_log tests/test_nfc_log.cpp)
target_link_libraries(test_nfc_log ${LIBNFC_LIBRARIES})

# 3) CAN 라우터 (요청 수신→BLE/NFC 트리거→결과 응답)
add_executable(rpi_can_router tests/rpi_can_router.cpp)
target_link_libraries(rpi_can_router ${BLUEZ_LIBRARIES} ${LIBNFC_LIBRARIES})
