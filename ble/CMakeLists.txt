# BLE 내장 라이브러리
add_library(sca_ble
  sca_ble_peripheral.cpp
)

target_include_directories(sca_ble PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${GLIB_INCLUDE_DIRS}
)

target_link_libraries(sca_ble PUBLIC ${GLIB_LIBRARIES})
