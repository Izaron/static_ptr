cmake_minimum_required(VERSION 3.14)
project(static_ptr VERSION 1.0)

# require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(include)

option(WITH_TESTS "build tests" ON)
if(WITH_TESTS)
    include_directories(include)
    add_subdirectory(test EXCLUDE_FROM_ALL)  # build only on 'make check'
endif(WITH_TESTS)

option(WITH_BENCHMARK "build benchmark" ON)
if(WITH_BENCHMARK)
    include_directories(benchmark)
    add_subdirectory(benchmark EXCLUDE_FROM_ALL)  # build only on 'make bm'
endif(WITH_BENCHMARK)
