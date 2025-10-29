add_library(camlib
  camera_runner.cpp
)
target_include_directories(camlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include)
