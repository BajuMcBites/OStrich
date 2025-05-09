#include "kernel_tests.h"

#include "../filesystem/filesys/fs_requests.h"
#include "atomic.h"
#include "bitmap.h"
#include "elf_loader.h"
#include "event.h"
#include "frame.h"
#include "fs_init.h"
#include "hash.h"
#include "heap.h"
#include "libk.h"
#include "locked_queue.h"
#include "mmap.h"
#include "printf.h"
#include "queue.h"
#include "ramfs.h"
#include "rand.h"
#include "ring_buffer.h"
#include "sched.h"
#include "sdio.h"
#include "stdint.h"
#include "string.h"
#include "swap.h"
#include "vm.h"

#define NUM_TIMES 1000

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

void swap_tests() {
    printf("Starting Swap Tests\n");

    Swap* swap = new Swap(32);
    alloc_frame(0, [=](uint64_t paddr) {
        uint64_t kvaddr = paddr_to_vaddr(paddr);
        int* temp = (int*)kvaddr;
        *temp = 1;
        temp += 1;
        *temp = 2;
        temp += 1;
        *temp = -1;

        swap->write_swap(1, (void*)kvaddr, [=]() {
            alloc_frame(0, [=](uint64_t new_paddr) {
                // printf("GOT PHYSICAL ADDRESS %X%X\n", new_paddr >> 32, new_paddr);
                void* new_kvaddr = (void*)paddr_to_vaddr(new_paddr);
                // printf("GOT PHYSICAL ADDRESS %X%X\n", new_paddr >> 32, new_paddr);
                swap->read_swap(1, new_kvaddr, [=]() {
                    int* temp = (int*)new_kvaddr;
                    K::assert(*temp == 1, "Read wrong value");
                    temp += 1;

                    K::assert(*temp == 2, "Read wrong value");
                    temp += 1;

                    K::assert(*temp == -1, "Read wrong value");

                    free_frame(paddr);
                    free_frame(new_paddr);

                    printf("Swap Test passed\n");
                });
            });
        });
    });
}

void bitmap_tests() {
    printf("Starting Bitmap Tests\n");

    Bitmap* bitmap = new Bitmap(30);

    printf("Filling up bitmap\n");
    for (int i = 0; i < 30; i++) {
        K::assert(bitmap->scan_and_flip() != -1, "Got -1, bitmap full when it shouldn't be");
    }

    printf("Freeing and reusing indexes\n");
    bitmap->free(15);
    bitmap->free(16);
    K::assert(bitmap->scan_and_flip() == 15, "Got -1, bitmap full when it shouldn't be");
    K::assert(bitmap->scan_and_flip() == 16, "Got -1, bitmap full when it shouldn't be");

    for (int i = 0; i < 30; i++) {
        bitmap->free(i);
    }

    for (int i = 0; i < 30; i++) {
        K::assert(bitmap->scan_and_flip() != -1, "Got -1, bitmap full when it shouldn't be");
    }

    printf("Bitmap tests passed\n");
}

void ring_buffer_tests() {
    RingBuffer<int>* buffer = new RingBuffer<int>(3);

    printf("Starting Ring Buffer Tests\n");

    printf("Starting read on empty buffer, should block\n");
    buffer->read([&](int result) {
        printf("Read %d\n", result);
        K::assert(result == 1, "Expected to read 1");
    });

    printf("Starting writes\n");
    buffer->write(1, [=]() {
        buffer->write(2, [&]() {
            buffer->write(3, [&]() {
                buffer->write(4, [&]() {
                    buffer->write(5, [&]() {
                        printf("This line shouldn't run until after 2 is read\n");
                        return;
                    });
                });
            });
        });
    });

    buffer->read([=](int result) {
        printf("Read %d\n", result);
        K::assert(result == 2, "Expected to read 2");
    });

    buffer->read([=](int result) {
        printf("Read %d\n", result);
        K::assert(result == 3, "Expected to read 3");
    });

    buffer->read([=](int result) {
        printf("Read %d\n", result);
        K::assert(result == 4, "Expected to read 4");
    });

    buffer->read([=](int result) {
        printf("Read %d\n", result);
        K::assert(result == 5, "Expected to read 5");
    });
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

template <typename T>
void test_function(T work) {
    int x = 10;
    create_event([=] { work(x); });
}

void foo() {
    auto bar = [](int a) { printf("I was given int:%d\n", a); };
    test_function(bar);
    printf("foo done\n");
}

void heapTests() {
    printf("Starting new/delete tests...\n");

    test_new_delete_basic();
    test_multiple_allocations();
    test_allocation_deletion_sequence();
    test_zero_allocation();
    test_nullptr_deletion();

    printf("All tests completed.\n");
    printf("foo test\n");
}

void test_ref_lambda() {
    static int a = 0;
    Function<void()> lambda = [&]() {
        a++;
        printf("%d current a\n", a);
    };
    for (int i = 0; i < 10; i++) {
        create_event(lambda);
    }
}
void test_val_lambda() {
    int a = 2;
    Function<void()> lambda = [=]() { printf("%d should print 2\n", a); };
    create_event(lambda, 1);
    create_event(lambda);
}

void event_loop_tests() {
    printf("Testing the event_loop..\n");
    test_ref_lambda();
    test_val_lambda();
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
        // printf("multi alloc\n"); check if all 600 ran
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
        // printf("pin frame\n"); check if all ran
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
    page_table = new PageTable();

    alloc_frame(0, [](uint64_t frame) {
        uint64_t user_vaddr = 0x800000;
        uint64_t lower_attributes = 0x404;
        page_table->map_vaddr(user_vaddr, frame, lower_attributes, [user_vaddr, frame]() {
            page_table->use_page_table();
            *((uint64_t*)user_vaddr) = 12345678;
            K::assert(*((uint64_t*)user_vaddr) == *((uint64_t*)paddr_to_vaddr(frame)),
                      "user virtual address not working");
            printf("basic_page_table_creation passed\n");
        });
    });
}

void mmap_test_file() {
    PCB* pcb = new PCB;

    uint64_t uvaddr = 0x9000;

    kopen("/dev/ramfs/test1.txt", [=](KFile* file) {
        printf("mmap_test_file(): we opened the file\n");
        mmap(pcb, 0x9000, PROT_WRITE | PROT_READ, MAP_PRIVATE, file, 0, PAGE_SIZE * 3 + 46, [=]() {
            load_mmapped_page(pcb, uvaddr, [=](uint64_t kvaddr) {
                pcb->page_table->use_page_table();
                char* kbuf = (char*)kvaddr;
                char* ubuf = (char*)uvaddr;

                printf("file mmap test: %s\n", kbuf);

                K::assert(K::strncmp(kbuf, "HELLO THIS IS A TEST FILE!!! OUR SIZE SHOULD BE 51!",
                                     60) == 0,
                          "mmap_test_file(): assertion of contents failed\n");
                K::assert(K::strncmp(ubuf, "HELLO THIS IS A TEST FILE!!! OUR SIZE SHOULD BE 51!",
                                     60) == 0,
                          "mmap_test_file(): assertion of contents failed\n");

                delete pcb;
            });
        });
    });
}

void mmap_test_no_reserve() {
    PCB* pcb = new PCB;

    uint64_t uvaddr = 0x9000;
    mmap(pcb, 0x9000, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, nullptr,
         0, PAGE_SIZE * 3 + 46, [=]() {
             load_mmapped_page(pcb, uvaddr + PAGE_SIZE, [=](uint64_t kvaddr) {
                 pcb->page_table->use_page_table();

                 char* kbuf = (char*)kvaddr;
                 char* ubuf = (char*)uvaddr + PAGE_SIZE;

                 K::strncpy(ubuf, "this is an mmap test", 30);

                 printf("no reserve mmap test: %s\n", ubuf);

                 K::assert(K::strncmp(kbuf, ubuf, 30) == 0, "no reserve mmap test failed\n");
                 delete pcb;
             });
         });
}

void mmap_shared_unreserved() {
    PCB* pcba = new PCB;
    PCB* pcbb = new PCB;

    uint64_t uvaddra = 0x90000;
    uint64_t uvaddrb = 0x70000;

    int shared_page_id = unreserved_id();
    Semaphore* sema = new Semaphore(-1);

    uint64_t* kvaddrs = (uint64_t*)kmalloc(sizeof(uint64_t) * 2);

    mmap_page(pcba, uvaddra, PROT_WRITE | PROT_READ, MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE,
              nullptr, 1, shared_page_id, [=]() {
                  load_mmapped_page(pcba, uvaddra, [=](uint64_t kvaddr) {
                      printf("kvaddr for a is %X%X\n", kvaddr >> 32, kvaddr);
                      kvaddrs[0] = kvaddr;
                      sema->up();
                  });
              });

    mmap_page(pcbb, uvaddrb, PROT_WRITE | PROT_READ, MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE,
              nullptr, 1, shared_page_id, [=]() {
                  load_mmapped_page(pcbb, uvaddrb, [=](uint64_t kvaddr) {
                      printf("kvaddr for b is %X%X\n", kvaddr >> 32, kvaddr);
                      kvaddrs[1] = kvaddr;
                      sema->up();
                  });
              });

    sema->down([=]() {
        K::assert(kvaddrs[0] == kvaddrs[1], "shared mmap test failed\n");
        delete kvaddrs;
        delete pcba;
        delete pcbb;
        delete sema;
    });
}

void user_paging_tests() {
    printf("starting user paging tests\n");
    basic_page_table_creation();
    mmap_test_no_reserve();
    mmap_test_file();
    mmap_test_file();
    mmap_shared_unreserved();
    printf("user paging tests complete\n");
}

uint64_t hash_func(int elem) {
    return (uint64_t)elem;
}

bool equals_func(int elem1, int elem2) {
    return elem1 == elem2;
}

void hash_test() {
    Rand rand;
    int keys[NUM_TIMES];

    HashMap<int, int> hash(hash_func, equals_func, 1);

    printf("Inserting %d random numbers\n", NUM_TIMES);
    for (int i = 0; i < NUM_TIMES; i++) {
        int randomNum = rand.random() % NUM_TIMES / 8;
        if (randomNum == 0) randomNum++;
        hash.put(i, randomNum);
        keys[i] = randomNum;
    }

    printf("Checking size is %d\n", NUM_TIMES);
    K::assert(hash.size == NUM_TIMES, "Got incorrect size\n");

    printf("Checking values are correct\n");
    for (int i = 0; i < NUM_TIMES; i++) {
        K::assert(keys[i] == hash.get(i), "Got incorrect value\n");
    }

    printf("Reinserting %d random numbers into hashmap\n", NUM_TIMES);
    for (int i = 0; i < NUM_TIMES; i++) {
        int randomNum = rand.random() % 101;
        if (randomNum == 0) randomNum++;
        hash.put(i, randomNum);
        keys[i] = randomNum;
    }

    printf("Checking size is %d\n", NUM_TIMES);
    K::assert(hash.size == NUM_TIMES, "Got incorrect size\n");

    printf("Checking values are correct\n");
    for (int i = 0; i < NUM_TIMES; i++) {
        if (keys[i] != hash.get(i)) {
            K::assert(keys[i] == hash.get(i), "Got incorrect value\n");
        }
    }

    printf("Removing first half of values\n");
    for (int i = 0; i < NUM_TIMES / 2; i++) {
        hash.remove(i);
    }

    printf("Checking size is %d\n", NUM_TIMES - NUM_TIMES / 2);
    K::assert(hash.size == NUM_TIMES - NUM_TIMES / 2, "Got incorrect size\n");

    printf("Checking values are null for first half of values\n");
    for (int i = 0; i < NUM_TIMES / 2; i++) {
        K::assert(hash.get(i) == 0, "Got non-null value at index\n");
    }

    printf("Removing all values\n");
    for (int i = 0; i < NUM_TIMES; i++) {
        hash.remove(i);
    }

    printf("Checking size is 0\n");
    K::assert(hash.size == 0, "Got incorrect size\n");

    printf("Checking values are null for all keys\n");
    for (int i = 0; i < NUM_TIMES; i++) {
        K::assert(hash.size == 0, "Got non-null value\n");
    }

    printf("Passed\n");
}

void ramfs_test_basic() {
    // only passes if test1 file is included
    int test1_index = get_ramfs_index("test1.txt");
    K::assert(test1_index >= 0, "ramfs couldn't find test1.txt\n");
    K::assert(ramfs_size(test1_index) == 51, "ramfs size method not working\n");
    char buffer[100];
    ramfs_read(buffer, 0, 51, test1_index);
    K::assert(K::strncmp(buffer, "HELLO THIS IS A TEST FILE!!! OUR SIZE SHOULD BE 51!", 51) == 0,
              "reading from ramfs file not working");
    printf("ramfs basic test passed\n");
}

void ramfs_big_file() {
    int test2_index = get_ramfs_index("test2.txt");
    char buffer[8192];
    ramfs_read(buffer, 24, 8192, test2_index);
    K::assert(
        K::strncmp(buffer,
                   "legendaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
                   60) == 0,
        "reading from ramfs big file not working");
    printf("ramfs big file test passed\n");
}

void ramfs_tests() {
    printf("start ramfs tests\n");
    ramfs_test_basic();
    ramfs_big_file();
    printf("end ramfs tests\n");
}

void semaphore_tests() {
    Semaphore* finish_sema = new Semaphore(-2);
    Semaphore* sema = new Semaphore(1);
    int* shared_value = (int*)kmalloc(sizeof(int));
    *shared_value = 0;

    Function<void()> func = [sema, finish_sema, shared_value]() {
        sema->down([sema, shared_value, finish_sema]() {
            for (int i = 0; i < 100; i++) {
                *shared_value = *shared_value + 1;
            }
            sema->up();
            finish_sema->up();
        });
    };

    Function<void()> check_func = [sema, finish_sema, shared_value]() {
        finish_sema->down([sema, finish_sema, shared_value]() {
            printf("sema test shared value is %d\n", *shared_value);
            printf("sema test finished with val\n");
            K::assert(*shared_value == 300, "race condition in semaphore test\n");
            delete sema;
            delete finish_sema;
            delete shared_value;
        });
    };

    create_event_core(func, 1);
    create_event_core(func, 2);
    create_event_core(func, 3);
    create_event(check_func);
}

void lock_tests() {
    Semaphore* finish_sema = new Semaphore(-3);
    Lock* lock = new Lock();
    int* shared_value = (int*)kmalloc(sizeof(int));
    *shared_value = 0;

    Function<void()> func = [lock, finish_sema, shared_value]() {
        lock->lock([lock, shared_value, finish_sema]() {
            for (int i = 0; i < 100; i++) {
                *shared_value = *shared_value + 1;
            }
            lock->unlock();
            finish_sema->up();
        });
    };

    Function<void()> check_func = [lock, finish_sema, shared_value]() {
        finish_sema->down([lock, finish_sema, shared_value]() {
            printf("lock test shared value is %d\n", *shared_value);
            K::assert(*shared_value == 400, "race condition in lock test\n");
            delete lock;
            delete finish_sema;
            delete shared_value;
        });
    };

    create_event_core(func, 1);
    create_event_core(func, 2);
    create_event_core(func, 3);
    create_event_core(func, 0);
    create_event(check_func);
}

void blocking_atomic_tests() {
    semaphore_tests();
    lock_tests();
}

void elf_load_test() {
    printf("start elf_load tests\n");
    int elf_index = get_ramfs_index("user_prog");
    K::assert(elf_index >= 0, "elf_load_test(): failed to find user_prog in ramfs");
    PCB* pcb = new PCB;
    const int sz = ramfs_size(elf_index);
    char* buffer = (char*)kmalloc(sz);
    ramfs_read(buffer, 0, sz, elf_index);
    Semaphore* sema = new Semaphore(1);
    void* new_pc = elf_load((void*)buffer, pcb, sema);
    UserTCB* tcb = new UserTCB();
    uint64_t sp = 0x0000fffffffff000;
    // only doing this because no eviction
    sema->down([=]() {
        mmap(pcb, sp - PAGE_SIZE, PF_R | PF_W, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, nullptr,
             0, PAGE_SIZE, [=]() {
                 load_mmapped_page(pcb, sp - PAGE_SIZE, [=](uint64_t kvaddr) {
                     pcb->page_table->use_page_table();
                     K::memset((void*)(sp - PAGE_SIZE), 0, PAGE_SIZE);
                     sema->up();
                 });
             });
    });
    sema->down([=]() {
        pcb->page_table->use_page_table();
        uint64_t sp = 0x0000fffffffff000;
        int argc = 0;
        const char* argv[0];
        uint64_t addrs[argc];
        for (int i = argc - 1; i >= 0; --i) {
            int len = K::strlen(argv[i]) + 1;
            sp -= len;
            addrs[i] = sp;
            K::memcpy((void*)sp, argv[i], len);
        }
        sp -= 8;
        *(uint64_t*)sp = 0;
        for (int i = argc - 1; i >= 0; --i) {
            sp -= 8;
            *(uint64_t*)sp = addrs[i];
        }
        // save &argv
        sp -= 8;
        *(uint64_t*)sp = sp + 8;

        // save argc
        sp -= 8;

        sp -= sp % 16;
        *(uint64_t*)sp = argc;
        tcb->context.sp = sp;
        sema->up();
    });
    tcb->pcb = pcb;
    // tcb->pcb->pid = 1;
    tcb->context.pc = (uint64_t)new_pc;
    tcb->context.x30 = (uint64_t)new_pc; /* this just to repeat the user prog again and again*/
    printf("%x this is pc\n", tcb->context.pc);
    sema->down([=]() {
        tcb->state = TASK_RUNNING;
        queue_user_tcb(tcb);
        kfree(buffer);
    });
}

// Two concurrent open / write / read / closes..
void kfs_simple_test() {
    printf("START KFS TESTS.\n");
    // Issue some FS requests to create some files.
    // For rn, kopen assumes the file already exists.
    // So we need to create the file first.
    constexpr int ROOT_DIR_INODE = 0;
    constexpr const char* DATA = "hello world";
    constexpr bool debug = false;

    int data_len = K::strlen(DATA) + 1;

    // Create file using issue_fs_request
    fs::issue_fs_create_file(ROOT_DIR_INODE, false, "test1.txt", 0, [=](fs::fs_response_t resp) {
        K::assert(resp.data.create_file.status == fs::FS_RESP_SUCCESS,
                  "kfs_simple_test(): failed to create file.");

        if constexpr (debug) {
            printf("kfs_simple_test(): created file with inode number %d\n",
                   resp.data.create_file.inode_index);
        }

        int inode_num = resp.data.create_file.inode_index;

        // Write data to file using issue_fs_request
        fs::issue_fs_write(inode_num, DATA, 0, data_len, [=](fs::fs_response_t resp) {
            K::assert(resp.data.write.bytes_written == data_len,
                      "kfs_simple_test(): failed to write to file.");
            K::assert(resp.data.write.status == fs::FS_RESP_SUCCESS,
                      "kfs_simple_test(): failed to write to file.");
            char* buffer = (char*)kmalloc(data_len);

            if constexpr (debug) {
                printf("kfs_simple_test(): wrote %d bytes to file with inode number %d\n",
                       resp.data.write.bytes_written, inode_num);
            }

            // Read data from file using issue_fs_request
            kopen("/test1.txt", [=](KFile* file) {
                if (!file) {
                    printf("kfs_simple_test(): failed to open file\n");
                    return;
                }

                if constexpr (debug) {
                    printf("kfs_simple_test(): k-opened file with inode number %d\n",
                           file->get_inode_number());
                }

                kread(file, 0, buffer, data_len, [=](int ret) {
                    K::assert(ret == data_len, "kfs_simple_test(): failed to read from file.");
                    K::assert(K::strcmp(buffer, DATA) == 0,
                              "kfs_simple_test(): data read from file is incorrect.");

                    printf("END KFS SIMPLE TEST (1/2) \n");

                    // Close the file.
                    kclose(file);
                    kfree(buffer);
                });
            });
        });
    });

    // Create file using issue_fs_request
    fs::issue_fs_create_file(ROOT_DIR_INODE, false, "test2.txt", 0, [=](fs::fs_response_t resp) {
        K::assert(resp.data.create_file.status == fs::FS_RESP_SUCCESS, "Failed to create file.");

        int inode_num = resp.data.create_file.inode_index;

        // Write data to file using issue_fs_request
        fs::issue_fs_write(inode_num, DATA, 0, data_len, [=](fs::fs_response_t resp) {
            K::assert(resp.data.write.status == fs::FS_RESP_SUCCESS,
                      "kfs_simple_test(): failed to write to file.");

            char* buffer = (char*)kmalloc(K::strlen(DATA) + 1);

            // Read data from file using issue_fs_request
            kopen("/test2.txt", [=](KFile* file) {
                if (!file) {
                    printf("kfs_simple_test(): failed to open file\n");
                    return;
                }

                kread(file, 0, buffer, data_len, [=](int ret) {
                    K::assert(ret == data_len, "kfs_simple_test(): failed to read from file.");
                    K::assert(K::strcmp(buffer, DATA) == 0,
                              "kfs_simple_test(): data read from file is incorrect.");

                    printf("END KFS SIMPLE TEST (2/2) \n");
                    // Close the file.
                    kclose(file);
                    kfree(buffer);
                });
            });
        });
    });
    printf("finished issuing kfs test\n");
}

void kfs_stress_test(int num_files) {
    constexpr const char* DATA = "hello world";
    int data_len = K::strlen(DATA) + 1;

    printf("START KFS STRESS TEST WITH %d FILES.\n", num_files);

    auto to_string = [](int i) {
        if (i == 0) {
            return string("0");
        }
        string s = "";
        while (i > 0) {
            s += (char)(i % 10 + '0');
            i /= 10;
        }
        return s;
    };

    constexpr int ROOT_DIR_INODE = 0;
    constexpr const char* DATA_2 = "goodbye world :(";

    // Create a large number of files.
    auto setup_files = [=](bool should_create, auto callback) {
        for (int i = 0; i < num_files; i++) {
            string filename = string("test") + to_string(i) + string(".txt");
            if (should_create) {
                fs::issue_fs_create_file(
                    ROOT_DIR_INODE, false, filename, 0, [=](fs::fs_response_t resp) {
                        K::assert(resp.data.create_file.status == fs::FS_RESP_SUCCESS,
                                  "kfs_stress_test(): failed to create file.");
                        if (i == num_files - 1) {
                            callback();
                        }
                    });
            } else {
                fs::issue_fs_remove_file(ROOT_DIR_INODE, filename, [=](fs::fs_response_t resp) {
                    K::assert(resp.data.remove_file.status == fs::FS_RESP_SUCCESS,
                              "kfs_stress_test(): failed to remove file.");
                    if (i == num_files - 1) {
                        callback();
                    }
                });
            }
        }
    };

    // Open all files.
    // Write to them.
    // Read from them.
    int* num_correct = (int*)kmalloc(sizeof(int));
    *num_correct = 0;
    auto run_test = [=]() {
        for (int i = 0; i < num_files; i++) {
            string filename = string("test") + to_string(i) + string(".txt");
            string file_to_open = "/" + filename;

            kopen(file_to_open, [=](KFile* file) {
                if (!file) {
                    printf("kfs_stress_test(): failed to open file %s\n", filename.c_str());
                    return;
                }

                kwrite(file, 0, DATA, data_len, [=](int ret) {
                    K::assert(ret == data_len, "kfs_stress_test(): failed to write to file.");
                    char* buffer = (char*)kmalloc(data_len);
                    kread(file, 0, buffer, data_len, [=](int ret) {
                        K::assert(ret == data_len, "kfs_stress_test(): failed to read from file.");

                        if (K::strcmp(buffer, DATA) == 0) {
                            (*num_correct)++;
                        } else {
                            printf("kfs_stress_test(): data read from file %s is incorrect.\n",
                                   filename.c_str());
                        }

                        // Close the file.
                        kclose(file);
                        kfree(buffer);

                        if (i == num_files - 1) {
                            printf("kfs_stress_test(): correctly read %d / %d files.\n",
                                   *num_correct, num_files);
                            printf("kfs_stress_test(): removing %d files.\n", num_files);
                            setup_files(/* removes num_files files */ false,
                                        [=](void) { printf("PASSED KFS STRESS TEST.\n"); });
                        }
                    });  // kread
                });      // kwrite
            });          // kopen
        }
    };

    setup_files(/* creates num_files files */ true, run_test);
}

// test we can open the same file twice and hit the open file cache.
void kfs_kopen_uses_cache_test() {
    printf("START KOPEN CACHE TESTS.\n");

    constexpr const char* FILENAME = "test69.txt";
    constexpr const char* FULL_PATH = "/test69.txt";
    constexpr const char* DATA = "hello world";
    int data_len = K::strlen(DATA) + 1;

    auto cleanup = [](KFile* file, int expected_status) {
        fs::issue_fs_remove_file(0, FILENAME, [=](fs::fs_response_t resp) {
            K::assert(resp.data.remove_file.status == expected_status,
                      "kfs_kopen_uses_cache_test(): failed to remove file.");
        });
        kclose(file);
    };

    fs::issue_fs_create_file(0, false, FILENAME, 0, [=](fs::fs_response_t resp) {
        K::assert(resp.data.create_file.status == fs::FS_RESP_SUCCESS,
                  "kfs_kopen_uses_cache_test(): failed to create file.");

        int inode_num = resp.data.create_file.inode_index;
        kopen(FULL_PATH, [=](KFile* file1) {
            if (!file1) {
                printf("kfs_kopen_uses_cache_test(): failed to open file1\n");
                return;
            }

            // file should be in memory now.
            int outer_inode_num = file1->get_inode_number();
            kopen(FULL_PATH, [=](KFile* file2) {
                if (!file2) {
                    printf("kfs_kopen_uses_cache_test(): failed to open file2\n");
                    return;
                }

                // return inode should be the same
                int inner_inode_num = file2->get_inode_number();
                K::assert(inner_inode_num == outer_inode_num,
                          "kfs_kopen_uses_cache_test(): inode numbers are different.");

                cleanup(file1, fs::FS_RESP_SUCCESS);
                cleanup(file2, fs::FS_RESP_ERROR_NOT_FOUND);

                printf("PASSED KOPEN CACHE TESTS.\n");
            });
        });
    });
}

void sd_stress_test() {
    printf("BEGIN SD STRESS TEST\n");
    constexpr int num_blocks = 8;

    for (int i = 0; i < 10000; i++) {
        create_event([i] {
            int num_blocks = 8;
            uint8_t* buffer = (uint8_t*)kmalloc(512 * num_blocks);
            sd_read_block(50 + (i % 500), buffer, num_blocks);
            kfree(buffer);
            if (i == 9999) {
                printf("SD STRESS TEST: done all reads\n");
            }
        });
    }

    uint8_t* buffer2 = (uint8_t*)kmalloc(512 * num_blocks);
    for (int i = 0; i < 512 * num_blocks; i++) {
        buffer2[i] = i;
    }

    for (int i = 0; i < 10000; i++) {
        create_event([i, buffer2] {
            sd_write_block(buffer2, 50 + (i % 500), num_blocks);
            if (i == 9999) {
                printf("SD STRESS TEST: done all writes\n");
                kfree(buffer2);
            }
        });
    }
}

void fs_syscalls_tests() {
    printf("Start FS syscalls tests!\n");
    int elf_index = get_ramfs_index("fs_syscall_test");
    K::assert(elf_index >= 0, "fs_syscalls_tests(): failed to find fs_syscall_test in ramfs");
    PCB* pcb = new PCB;
    const int sz = ramfs_size(elf_index);
    char* buffer = (char*)kmalloc(sz);
    ramfs_read(buffer, 0, sz, elf_index);
    Semaphore* sema = new Semaphore(1);
    void* new_pc = elf_load((void*)buffer, pcb, sema);
    UserTCB* tcb = new UserTCB();
    uint64_t sp = 0x0000fffffffff000;
    // only doing this because no eviction
    sema->down([=]() {
        mmap(pcb, sp - PAGE_SIZE, PF_R | PF_W, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, nullptr,
             0, PAGE_SIZE, [=]() {
                 load_mmapped_page(pcb, sp - PAGE_SIZE, [=](uint64_t kvaddr) {
                     pcb->page_table->use_page_table();
                     K::memset((void*)(sp - PAGE_SIZE), 0, PAGE_SIZE);
                     sema->up();
                 });
             });
    });
    sema->down([=]() {
        pcb->page_table->use_page_table();
        uint64_t sp = 0x0000fffffffff000;
        int argc = 0;
        const char* argv[0];
        uint64_t addrs[argc];
        for (int i = argc - 1; i >= 0; --i) {
            int len = K::strlen(argv[i]) + 1;
            sp -= len;
            addrs[i] = sp;
            K::memcpy((void*)sp, argv[i], len);
        }
        sp -= 8;
        *(uint64_t*)sp = 0;
        for (int i = argc - 1; i >= 0; --i) {
            sp -= 8;
            *(uint64_t*)sp = addrs[i];
        }
        // save &argv
        sp -= 8;
        *(uint64_t*)sp = sp + 8;

        // save argc
        sp -= 8;
        *(uint64_t*)sp = argc;
        tcb->context.sp = sp;
        sema->up();
    });
    tcb->pcb = pcb;
    // tcb->pcb->pid = 1;
    tcb->context.pc = (uint64_t)new_pc;
    tcb->context.x30 = (uint64_t)new_pc; /* this just to repeat the user prog again and again*/
    printf("%x this is pc\n", tcb->context.pc);
    sema->down([=]() {
        tcb->state = TASK_RUNNING;
        queue_user_tcb(tcb);
        kfree(buffer);
    });
}
