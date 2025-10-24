# CMakeLists.txt (핵심 부분만)
cmake_minimum_required(VERSION 3.16)
project(SCA-Core CXX)
set(CMAKE_CXX_STANDARD 17)

add_library(can_core
  can_api.cpp
  canmessage.cpp
  linux_adapter.cpp
  adapter_factory.cpp
  channel.cpp          # ← 반드시 포함
  adapter_debug.cpp    # ← 아래 예제 제공. 실제 사용 환경에 맞춰 교체 가능
)

target_include_directories(can_core PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(can_core PRIVATE pthread)

add_executable(smoke_test tests/smoke_test.cpp)
target_link_libraries(smoke_test PRIVATE can_core pthread)

add_executable(router_canapi tests/router_canapi.cpp)
target_link_libraries(router_canapi PRIVATE can_core pthread)
