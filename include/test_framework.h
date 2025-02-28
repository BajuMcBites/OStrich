/* test_framework.h */
#ifndef TEST_FRAMEWORK_H_
#define TEST_FRAMEWORK_H_

#include "printf.h"
#include "libk.h"

/*
To add a test suite, do the following:

Add this line in runKernelTests()::kernel_tests.cpp
    RUN_TEST_SUITE(basic_test_suite, true) // verbose mode = true.

tests/test_suites.h: Declare test suite.
   test_suite_stats basic_test_suite(bool); 

my_test_suite.cpp: Define test suite.
    #include "tests/test_suites.h"

    // Define test cases.
    TEST_CASE(should_pass) {
        EXPECT_TRUE(1 == 1, "They should be equal.");
        ASSERT_TRUE(5 != 4, "Shouldn't be equal.");
        return true;
    }

    // Define test suite (put at the end of the file).
    BEGIN_TEST_SUITE(basic_test_suite)
        TEST_CASE(test_should_pass)
    END_TEST_SUITE
*/

struct suite_stats {
    int passed;
    int total;
} typedef test_suite_stats;

#define BEGIN_TEST_SUITE(suite_name) \
    test_suite_stats test_suite_##suite_name(bool verbose) { \
        test_suite_stats suite_stats; \
        suite_stats.passed = 0; \
        suite_stats.total = 0; \
        do  /* Begin user test case block */ {           \
            /* User's test cases go here */               \

#define END_TEST_SUITE \
    } while(0); \
    return suite_stats; \
    }

#define TEST_CASE(name) \
    bool test_case_##name() \

#define ADD_TEST(test_function) \
   if (test_case_##test_function()) { \
        suite_stats.passed++; \
    } else if (verbose) { \
        printf("Failed Test: %s\n", #test_function); \
    } \
    suite_stats.total++; \
    if (verbose) printf("Passed Test: %s\n", #test_function);

#define DECLARE_TEST_SUITE(suite_name) \
    test_suite_stats test_suite_##suite_name(bool verbose);

#define RUN_TEST_SUITE(suite_name, verbose) \
    printf("Running Test Suite '%s':\n", #suite_name); \
    suite_results = test_suite_##suite_name(verbose); \
    printf("Passed %d / %d tests.\n", suite_results.passed, suite_results.total); \
    printf("Finished Running Suite '%s'\n\n\n", #suite_name); \

// Use do-while loop to avoid pre-processor expansion issues
#define EXPECT_TRUE(condition, msg) do {                    \
    if (!(condition)) {                                     \
        printf("Failed: %s, (File: %s, Line: %d)\n",         \
                (msg), __FILE__, __LINE__);                  \
    }                                                       \
} while(0)

#define ASSERT_TRUE(condition, msg) do {                    \
    if (!(condition)) {                                     \
        printf("Failed: %s, (File: %s, Line: %d)\n",         \
                (msg), __FILE__, __LINE__);                  \
        K::assert(0, "ASSERTION_FAILED!!!");                \
    }                                                       \
} while(0)

#endif /* TEST_FRAMEWORK_H_ */