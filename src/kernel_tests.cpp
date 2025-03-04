#include "kernel_tests.h"

#include "event_loop.h"
#include "frame.h"
#include "heap.h"
#include "libk.h"
#include "printf.h"
#include "queue.h"
#include "sched.h"
#include "stdint.h"
#include "vm.h"
#include "atomic.h"

PageTable* page_table;

void test_new_delete_basic() {
    printf("Test 1: Basic Allocation and Deletion\n");

    int* p = new int;
    K::assert(p != nullptr, "new int returned nullptr");
    *p = 42;
    K::assert(*p == 42, "Value mismatch after new int");

    delete p;
    printf("Test 1 passed.\n");
}

void test_multiple_allocations() {
    printf("Test 2: Multiple Allocations\n");

    int* p1 = new int;
    int* p2 = new int;
    int* p3 = new int;

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

void test_allocation_deletion_sequence() {
    printf("Test 3: Allocation, Deletion, and Reallocation\n");

    int* p1 = new int;
    delete p1;
    int* p2 = new int;

    K::assert(p1 == p2, "Memory was not reused after deletion");

    delete p2;
    printf("Test 3 passed.\n");
}

void test_zero_allocation() {
    printf("Test 4: Zero Allocation\n");

    char* p = new char[0];
    K::assert(p != nullptr, "new char[0] returned nullptr");

    delete[] p;
    printf("Test 4 passed.\n");
}

void test_nullptr_deletion() {
    printf("Test 5: Null Pointer Deletion\n");

    int* p = nullptr;
    delete p;
    printf("Test 5 passed.\n");
}

void test_event(void* arg) {
    printf("new event dropped: %s\n", (char*)arg);
}

void heapTests() {
    printf("Starting new/delete tests...\n");

    test_new_delete_basic();
    test_multiple_allocations();
    test_allocation_deletion_sequence();
    test_zero_allocation();
    test_nullptr_deletion();

    printf("All tests completed.\n");
}

void test_ref_lambda() {
    static int a = 0;
    Function<void()> lambda = [&]() {
        a++;
        printf("%d current a\n", a);
    };
    for (int i = 0; i < 10; i++) {
        create_event(lambda, 1);
    }
}
void test_val_lambda() {
    int a = 2;
    Function<void()> lambda = [=]() { printf("%d should print 2\n", a); };
    create_event(lambda, 1);
}

void event_loop_tests() {
    printf("Testing the event_loop..\n");
    test_ref_lambda();
    test_val_lambda();
    printf("All tests completed.\n");
}

void queue1() {
    queue<int>* q = (queue<int>*)malloc(sizeof(queue<int>));
    q->push(5);
    q->push(3);
    q->push(2);
    q->push(1);
    printf("size %d\n", q->size());  // 4
    printf("%d ", q->top());         // 5
    q->pop();
    printf("%d ", q->top());  // 3
    q->pop();
    printf("%d ", q->top());  // 2
    q->pop();
    printf("%d\n", q->top());  // 1
    q->pop();
    printf("size %d\n", q->size());       // 0
    printf("empty is %d\n", q->empty());  // 1
    free(q);
}

void queue_test() {
    printf("Testing the queue implementation..\n");
    queue1();
    printf("All tests completed.\n");
}

void frame_alloc_tests() {
    printf("Starting frame allocator tests...\n");

    test_frame_alloc_simple();
    test_frame_alloc_multiple();
    test_pin_frame();

    printf("All frame allocator tests completed.\n");
}

void test_frame_alloc_simple() {
    Function<void(uint64_t)> lambda = [](uint64_t a) {
        printf("got address %d\n", a);
        K::assert(a, "got null frame");
    };
    alloc_frame(0x3, lambda);
    printf("test_frame_alloc_simple passed\n");
}

void test_frame_alloc_multiple() {
    Function<void(uint64_t)> lambda = [](uint64_t a) {
        K::assert(a, "got null frame");
        K::assert(free_frame(a), "could not free frame");
    };
    for (int i = 0; i < 600; i++) {
        alloc_frame(0x0, lambda);
    }
    printf("test_frame_alloc_multiple passed\n");
}

void test_pin_frame() {
    Function<void(uint64_t)> lambda = [](uint64_t a) {
        K::assert(a, "got null frame");
        pin_frame(a);
        K::assert(!free_frame(a), "freed pinned frame");
        unpin_frame(a);
        K::assert(free_frame(a), "could not free unpinned frame");
    };
    for (int i = 0; i < 300; i++) {
        alloc_frame(0x0, lambda);
    }
    printf("test_pin_frame passed\n");
}

void basic_page_table_creation() {
    page_table = new PageTable([]() {
        printf("we have allocated a page table\n");
        alloc_frame(0, [](uint64_t frame) {
            uint64_t user_vaddr = 0x800000;
            uint16_t lower_attributes = 0x404;
            page_table->map_vaddr(user_vaddr, frame, lower_attributes, [user_vaddr, frame]() {
                page_table->use_page_table();
                *((uint64_t*)user_vaddr) = 12345678;
                K::assert(*((uint64_t*)user_vaddr) == *((uint64_t*)paddr_to_vaddr(frame)),
                          "user virtual address not working");
                printf("basic_page_table_creation passed\n");
            });
        });
    });
}

void user_paging_tests() {
    printf("starting user paging tests\n");
    basic_page_table_creation();
    printf("user paging tests complete\n");
}

void semaphore_tests() {
    Semaphore* sema = new Semaphore(1);

    Function<void()> func1 = [sema]() {
        printf("in func1\n");
        for (int i = 0; i < 5; i++) {
            printf("func1 on core %d\n", getCoreID());
        }
        printf("getting semaphore from func1\n");
        sema->down([sema]() {
            printf("we have semaphore in func1\n");
            for (int i = 0; i < 5; i++) {
                printf("we are looping in func1\n");
            }
            printf("we still have semaphore in func1\n");
            sema->up();
        });
    };

    Function<void()> func2 = [sema]() {
        printf("in func2\n");
        for (int i = 0; i < 5; i++) {
            printf("func2 on core %d\n", getCoreID());
        }
        printf("getting semaphore from func2\n");
        sema->down([sema]() {
            printf("we have semaphore in func2\n");
            for (int i = 0; i < 5; i++) {
                printf("we are looping in func2\n");
            }
            printf("we still have semaphore in func2\n");
            sema->up();
        });
    };

    create_event_core(func1, 1, 2);
    create_event_core(func2, 1, 3);
}

void blocking_atomic_tests() {
    semaphore_tests();
}

