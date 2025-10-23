cmake_minimum_required(VERSION 3.13)
project(cuckoo_sip CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (NOT CMAKE_CXX_FLAGS)
  set(CMAKE_CXX_FLAGS "-O3 -march=native -DNDEBUG")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package(Threads REQUIRED)

add_library(cuckoo_sip_lib
  src/siphash.cc
  cuckoo/graph.cc
  cuckoo/lean_solver.cc
  cuckoo/mean_solver.cc
  verify/verify.cc
  bench/bench.cc
)

target_include_directories(cuckoo_sip_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(cuckoo_sip_lib PUBLIC Threads::Threads)

add_executable(cuckoo_sip cli/main.cc)

target_link_libraries(cuckoo_sip PRIVATE cuckoo_sip_lib Threads::Threads)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(cuckoo_sip_lib PRIVATE -Wall -Wextra -Wno-unused-parameter)
  target_compile_options(cuckoo_sip PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
