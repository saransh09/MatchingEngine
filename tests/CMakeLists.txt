
if (NOT BUILD_TESTS)
  return()
endif()


# Test executable
add_executable(order_book_test
order_book_test.cpp
matching_test.cpp
order_generator_test.cpp
integration_test.cpp
order_book_flat_sorted_test.cpp
matching_test_flat.cpp
)

target_link_libraries(
  order_book_test PRIVATE core gtest gtest_main
)

include(GoogleTest)
gtest_discover_tests(
  order_book_test
)
