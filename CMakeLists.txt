cmake_minimum_required(VERSION 3.12)
project(cuckoo_sip CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow user to override build type; default to Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(cuckoo_sip
  src/siphash12.cc
  src/util.cc
  cuckoo/graph.cc
  cuckoo/lean_solver.cc
  cuckoo/mean_solver.cc
  verify/verify.cc
  bench/bench.cc
  cli/main.cc
)

# Add project root so includes like "cuckoo/graph.h" and "bench/bench.h" resolve
target_include_directories(cuckoo_sip PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} src cuckoo verify bench)

target_compile_options(cuckoo_sip PRIVATE -O3 -Wall -Wextra -Wno-unused-parameter)

# Threads for std::thread
find_package(Threads REQUIRED)
target_link_libraries(cuckoo_sip PRIVATE Threads::Threads)
