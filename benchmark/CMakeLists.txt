project(bm)

include(FetchContent)
FetchContent_Declare(
  googlebenchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.6.1.zip
)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(bm benchmark.cc)
target_link_libraries(bm benchmark::benchmark)
