find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0)

# 라이브러리
add_library(blelib
  sca_ble_peripheral_lib.cpp
)
target_include_directories(blelib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include)
target_include_directories(blelib PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(blelib PRIVATE ${GLIB_LIBRARIES})
target_compile_options(blelib PRIVATE ${GLIB_CFLAGS_OTHER})

# 실행파일 (main)
add_executable(sca_ble_peripheral
  main_ble.cpp
)
target_link_libraries(sca_ble_peripheral PRIVATE blelib)

# 빌드 후 루트에 복사(외부 실행 쓰는 경우 편의)
add_custom_command(TARGET sca_ble_peripheral POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:sca_ble_peripheral> ${CMAKE_SOURCE_DIR}/
)
