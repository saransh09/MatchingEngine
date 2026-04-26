
if (NOT BUILD_TESTS)
  return()
endif()


# Test executable
add_executable(order_book_test
order_book_test.cpp
matching_test.cpp
)

target_link_libraries(
  order_book_test PRIVATE core gtest gtest_main
)

include(GoogleTest)
gtest_discover_tests(
  order_book_test
)
