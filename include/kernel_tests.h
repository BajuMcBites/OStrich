#ifndef _KERNEL_TESTS_H
#define _KERNEL_TESTS_H

extern "C" void heapTests();
void event_loop_tests();
void queue_test();
void frame_alloc_tests();
void test_frame_alloc_simple();
void test_frame_alloc_multiple();
void test_pin_frame();
void user_paging_tests();
void hash_test();
void ramfs_tests();
void blocking_atomic_tests();
void ring_buffer_tests();

#endif /*_KERNEL_TESTS_H */