cmake_minimum_required(VERSION 3.20)

project(MatchingEngineBenchmarks LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(matching_benchmark
  matching_benchmark.cpp
)

add_executable(matching_benchmark_flat
  matching_benchmark_flat.cpp
)

target_link_libraries(matching_benchmark
  PRIVATE
  core
  benchmark::benchmark
  benchmark::benchmark_main
)

target_link_libraries(matching_benchmark_flat
  PRIVATE
  core
  benchmark::benchmark
  benchmark::benchmark_main
)

target_compile_options(matching_benchmark PRIVATE -flto)
target_link_options(matching_benchmark PRIVATE -flto)

target_compile_options(matching_benchmark_flat PRIVATE -flto)
target_link_options(matching_benchmark_flat PRIVATE -flto)
