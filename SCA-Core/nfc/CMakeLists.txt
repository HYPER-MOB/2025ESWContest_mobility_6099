# NFC 내장 라이브러리
add_library(sca_nfc
  nfc_reader.cpp
)

target_include_directories(sca_nfc PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LIBNFC_INCLUDE_DIRS}
)

target_link_libraries(sca_nfc PUBLIC ${LIBNFC_LIBRARIES})
