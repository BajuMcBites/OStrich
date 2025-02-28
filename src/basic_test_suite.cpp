#include "tests/test_suites.h"

// Define test cases.
TEST_CASE(should_pass) {
    EXPECT_TRUE(1 == 1, "They should be equal.");
    ASSERT_TRUE(5 != 4, "Shouldn't be equal.");
    return true;
}

// Define test suite (put at the end of the file).
BEGIN_TEST_SUITE(basic_test_suite)
    ADD_TEST(should_pass)
END_TEST_SUITE

