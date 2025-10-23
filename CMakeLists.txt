cmake_minimum_required(VERSION 3.12)
project(cuckoo_sip CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-O3)

add_library(siphash src/siphash.cc)
target_include_directories(siphash PUBLIC ${CMAKE_SOURCE_DIR})

add_library(graph cuckoo/graph.cc)
target_link_libraries(graph PUBLIC siphash)
target_include_directories(graph PUBLIC ${CMAKE_SOURCE_DIR})

add_library(verify verify/verify.cc)
target_link_libraries(verify PUBLIC graph siphash)
target_include_directories(verify PUBLIC ${CMAKE_SOURCE_DIR})

add_library(lean_solver cuckoo/lean_solver.cc)
target_link_libraries(lean_solver PUBLIC graph siphash verify)
target_include_directories(lean_solver PUBLIC ${CMAKE_SOURCE_DIR})

add_library(mean_solver cuckoo/mean_solver.cc)
target_link_libraries(mean_solver PUBLIC graph siphash verify)
target_include_directories(mean_solver PUBLIC ${CMAKE_SOURCE_DIR})

add_library(bench bench/bench.cc)
target_link_libraries(bench PUBLIC graph siphash verify lean_solver mean_solver)
target_include_directories(bench PUBLIC ${CMAKE_SOURCE_DIR})

add_executable(cuckoo_sip cli/main.cc)
target_link_libraries(cuckoo_sip PRIVATE bench graph siphash verify lean_solver mean_solver)
