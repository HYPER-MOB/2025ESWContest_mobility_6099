add_library(nfclib
  nfc_reader.cpp
)
target_include_directories(nfclib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include)

# libnfc ÇÊ¼ö
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNFC REQUIRED libnfc)
target_include_directories(nfclib PRIVATE ${LIBNFC_INCLUDE_DIRS})
target_link_libraries(nfclib PRIVATE ${LIBNFC_LIBRARIES})
target_compile_options(nfclib PRIVATE ${LIBNFC_CFLAGS_OTHER})
target_compile_definitions(nfclib PRIVATE USE_LIBNFC)
