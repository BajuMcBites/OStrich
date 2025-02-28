#ifndef _KERNEL_TESTS_H
#define _KERNEL_TESTS_H

#include "test_framework.h"

extern "C" void heapTests();
void runKernelTests();
void event_loop_tests();
void queue_test();
void frame_alloc_tests();
void test_frame_alloc_simple();
void test_frame_alloc_multiple();
void test_pin_frame();

// Test Suite Declarations.
test_suite_stats basic_test_suite(bool);

#endif /*_KERNEL_TESTS_H */