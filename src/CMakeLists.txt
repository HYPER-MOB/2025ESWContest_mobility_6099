# 메인 실행파일 (CAN 수신 → Sequencer → BLE/NFC 호출)
add_executable(rpi_can_router
  main.cpp
  sequencer.cpp
)

target_include_directories(rpi_can_router PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(rpi_can_router
  PRIVATE can_core sca_ble sca_nfc camera_runner
          ${GLIB_LIBRARIES} ${LIBNFC_LIBRARIES}
)
