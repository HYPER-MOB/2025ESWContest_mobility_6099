cmake_minimum_required(VERSION 3.16)
project(can_test CXX)
set(CMAKE_CXX_STANDARD 17)

add_library(can_core
  can_api.cpp
  canmessage.cpp
  # channel/adapter 백엔드 추가 시 여기에
)
target_include_directories(can_core PUBLIC ${CMAKE_SOURCE_DIR})

add_executable(router_canapi tests/router_canapi.cpp)
target_link_libraries(router_canapi PRIVATE can_core pthread)

add_executable(smoke_test tests/smoke_test.cpp)
target_link_libraries(smoke_test PRIVATE can_core pthread)
