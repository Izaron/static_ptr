# enable gtest (for testing)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/c9461a9b55ba954df0489bab6420eb297bed846b.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

set(tests
    test_buffer_size
    test_derived
)

include(GoogleTest)

foreach(test IN LISTS tests)
    add_executable(${test} ${test}.cc)
    target_link_libraries(${test} gtest_main)
    add_test(${test} ${test})
    add_dependencies(check ${test})
    gtest_discover_tests(${test})
endforeach()
