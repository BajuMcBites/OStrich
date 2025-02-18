#include "libk.h"
#include "stdint.h"
#include "heap.h"
#include "printf.h"
#include "kernel_tests.h"
#include "sched.h"
#include "queue.h"

void test_new_delete_basic()
{
    printf("Test 1: Basic Allocation and Deletion\n");

    int *p = new int;
    K::assert(p != nullptr, "new int returned nullptr");
    *p = 42;
    K::assert(*p == 42, "Value mismatch after new int");

    delete p;
    printf("Test 1 passed.\n");
}

void test_multiple_allocations()
{
    printf("Test 2: Multiple Allocations\n");

    int *p1 = new int;
    int *p2 = new int;
    int *p3 = new int;

    K::assert(p1 != nullptr, "p1 is nullptr");
    K::assert(p2 != nullptr, "p2 is nullptr");
    K::assert(p3 != nullptr, "p3 is nullptr");

    K::assert(p1 != p2, "p1 and p2 have the same address");
    K::assert(p2 != p3, "p2 and p3 have the same address");
    K::assert(p1 != p3, "p1 and p3 have the same address");

    delete p1;
    delete p2;
    delete p3;
    printf("Test 2 passed.\n");
}

void test_allocation_deletion_sequence()
{
    printf("Test 3: Allocation, Deletion, and Reallocation\n");

    int *p1 = new int;
    delete p1;
    int *p2 = new int;

    K::assert(p1 == p2, "Memory was not reused after deletion");

    delete p2;
    printf("Test 3 passed.\n");
}

void test_zero_allocation()
{
    printf("Test 4: Zero Allocation\n");

    char *p = new char[0];
    K::assert(p != nullptr, "new char[0] returned nullptr");

    delete[] p;
    printf("Test 4 passed.\n");
}

void test_nullptr_deletion()
{
    printf("Test 5: Null Pointer Deletion\n");

    int *p = nullptr;
    delete p;
    printf("Test 5 passed.\n");
}

void test_event(void* arg) {
    printf("new event dropped: %s\n", (char*)arg);
}

void heapTests()
{

    printf("Starting new/delete tests...\n");

    test_new_delete_basic();
    test_multiple_allocations();
    test_allocation_deletion_sequence();
    test_zero_allocation();
    test_nullptr_deletion();

    printf("All tests completed.\n");
}

void test_events_sequential_order() {
    create_event(test_event, (void*) "event 1", 2);
    create_event(test_event, (void*) "event 2", 2);
    create_event(test_event, (void*) "event 3", 2);
}

void test_events_priority_order() {
    create_event(test_event, (void*) "event 1", 3);
    create_event(test_event, (void*) "event 2", 2);
    create_event(test_event, (void*) "event 3", 1);
}

void test_lambda() {
    auto greet = [](void* a) {
        printf("hello lambda %s", (char*) a);
    };
    create_event(greet, (void*) "lambda1", 3);
}

void scheduler_tests() {
    printf("Testing the scheduler..\n");
    //test_events_sequential_order();
    // test_events_priority_order();
    test_lambda();
    printf("All tests completed.\n");
}

void queue1() {
    queue<int>* q = (queue<int>*) malloc(sizeof(queue<int>));
    q->push(5);
    q->push(3);
    q->push(2);
    q->push(1);
    printf("size %d\n", q->size()); // 4
    printf("%d ", q->top()); // 5
    q->pop();
    printf("%d ", q->top()); // 3
    q->pop();
    printf("%d ", q->top()); // 2
    q->pop();
    printf("%d\n", q->top()); // 1
    q->pop();
    printf("size %d\n", q->size()); // 0
    printf("empty is %d\n", q->empty()); // 1
    free(q);
}

void queue_test() {
    printf("Testing the queue implementation..\n");
    queue1();
    printf("All tests completed.\n");
}