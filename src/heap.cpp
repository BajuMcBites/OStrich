#include "heap.h"

#include "printf.h"
#include "stdint.h"
// #include <stdint.h>
// #include "blocking_lock.h"
#include "atomic.h"
#include "libk.h"

static SpinLock* theLock = nullptr;
// MY HEAP
// A sample pointer to the start of the free list.
memory_block_t* free_head;

namespace heapHelpers {
/*
 * is_allocated - returns true if a block is marked as allocated.
 */
bool is_allocated(memory_block_t* block) {
    // assert(block != nullptr);
    return block->block_size_alloc & 0x1;
}

/*
 * allocate - marks a block as allocated.
 */
void allocate(memory_block_t* block) {
    // assert(block != nullptr);
    block->block_size_alloc |= 0x1;
}

/*
 * deallocate - marks a block as unallocated.
 */
void deallocate(memory_block_t* block) {
    // assert(block != nullptr);
    block->block_size_alloc &= ~0x1;
}

/*
 * get_size - gets the size of the block.
 */
size_t get_block_size(memory_block_t* block) {
    // assert(block != nullptr);
    return block->block_size_alloc & ~(ALIGNMENT - 1);
}

/*
 * get_next - gets the next block.
 */
memory_block_t* get_next(memory_block_t* block) {
    // assert(block != nullptr);
    return block->next;
}

/*
 * put_block - puts a block struct into memory at the specified address.
 * Initializes the size and allocated fields, along with NUlling out the next
 * field.
 */
void put_block(memory_block_t* block, size_t size, bool alloc) {
    // assert(block != nullptr);
    // assert(size % ALIGNMENT == 0);
    // assert(alloc >> 1 == 0);
    block->block_size_alloc = size | alloc;
    block->next = nullptr;
}

/*
 * get_payload - gets the payload of the block.
 */
void* get_payload(memory_block_t* block) {
    // assert(block != nullptr);
    return (void*)(block + 1);
}

/*
 * get_block - given a payload, returns the block.
 */
memory_block_t* get_block(void* payload) {
    // assert(payload != nullptr);
    return ((memory_block_t*)payload) - 1;
}

/*
 * The following are helper functions that can be implemented to assist in your
 * design, but they are not required.
 */

/*
 * find - finds a free block that can satisfy the umalloc request.
 */
memory_block_t* find(size_t size) {
    memory_block_t* cur = free_head;
    memory_block_t* best = nullptr;
    while (cur != nullptr) {
        if (best == nullptr && cur->block_size_alloc >= size) {
            best = cur;
        } else if (best != nullptr && cur->block_size_alloc < best->block_size_alloc &&
                   cur->block_size_alloc >= size) {
            best = cur;
        }
        if (best != nullptr) {
            if (best->block_size_alloc == size) return best;
        }
        cur = cur->next;
    }
    return best;
}

/*
 * extend - extends the heap if more memory is required.
 * returns memory block that we added.
 */
memory_block_t* extend(size_t size) {
    K::assert(false, "we are out of heap space");
    return nullptr;
    /*
    size = ALIGN(size);
    if (size > PAGESIZE)
    {
        size = PAGESIZE;
    }
    memory_block_t *increase = (memory_block_t *)alloc_frame(size);
    deallocate(increase);
    increase->next = nullptr;
    increase->block_size_alloc = size;
    if (free_head == nullptr) // completely out of free mem, need a new head
    {
        free_head = increase;
    }
    else if (increase < free_head)
    {
        increase->next = free_head;
        free_head = increase;
        return increase;
    }
    else
    {
        memory_block_t *cur = free_head;
        while (cur->next != nullptr && cur->next < increase)
        { // add added mem to the end
            cur = cur->next;
        }
        increase->next = cur->next;
        cur->next = increase;
    }
    return increase;
    */
}

/*
 * split - splits a given block in parts, one allocated, one free.
 * handles all the freeblock list linking. return the block that is allocated.
 */
memory_block_t* split(memory_block_t* block, size_t size) {
    // new address of the block we want to allocate
    memory_block_t* toAllocate = (memory_block_t*)((char*)block + block->block_size_alloc - size);
    // the size of the new head is the remaining memory space in the original
    // block
    block->block_size_alloc = block->block_size_alloc - size;
    allocate(toAllocate);
    toAllocate->block_size_alloc = size;
    toAllocate->next = nullptr;
    return toAllocate;
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
memory_block_t* coalesce(memory_block_t* block) {
    memory_block_t* cur = free_head;
    while (cur) {
        if (cur->next != nullptr &&
            cur->next == (memory_block_t*)((char*)cur + cur->block_size_alloc)) {  // neighbors !
            cur->block_size_alloc = cur->block_size_alloc + cur->next->block_size_alloc;
            cur->next = cur->next->next;
        }
        cur = cur->next;
    }
    return block;
}
};  // namespace heapHelpers

/*
 * uinit - Used initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
void uinit(void* base, size_t bytes) {
    using namespace heapHelpers;
    free_head = (memory_block_t*)base;
    printf("| heap range 0x%x 0x%x\n", (uint64_t)free_head, (uint64_t)free_head + bytes);
    // free_head = (memory_block_t *)csbrk(PAGESIZE * 4);
    if (free_head == nullptr) {
        // return -1;
    }
    put_block(free_head, bytes, false);
    free_head->next = nullptr;
    // initialize lock
    theLock = new SpinLock();
}

/*
 * kmalloc -  allocates size bytes and returns a pointer to the allocated
 * memory.
 */
void* kmalloc(size_t size) {
    using namespace heapHelpers;
    // lock
    LockGuardP g{theLock};
    size = ALIGN(size + sizeof(memory_block_t));
    // we have a valid block size. now we can allocate
    memory_block_t* validBlock = find(size);
    while (validBlock == nullptr) {
        coalesce(free_head);  // check for another block
        validBlock = find(size);
        if (validBlock == nullptr) {
            // OUT OF HEAP MEMORY. I WILL SPIN 4 EVER UNTIL EXTEND WORKS
            // I should block and try again but no blocking locks either
            memory_block_t* temp = extend(size * 2);
            if (temp && temp->block_size_alloc >= size) {
                validBlock = temp;
            }
        }
    }
    if (validBlock->block_size_alloc >= 2 * size) {
        validBlock = split(validBlock, size);
    } else {
        if (validBlock == free_head)  // unlink head
        {
            free_head = free_head->next;
        } else  // unlink block inside of list
        {
            memory_block_t* cur = free_head->next;
            memory_block_t* prev = free_head;
            while (cur < validBlock) {
                prev = cur;
                cur = cur->next;
            }
            prev->next = cur->next;
        }
    }
    allocate(validBlock);  // mark block as allocated
    validBlock->next = nullptr;

    return get_payload(validBlock);
}

/**
 * kcalloc -  count amount of size objects all zeroed.
 */
void* kcalloc(size_t count, size_t size) {
    void* block = kmalloc(count * size);
    K::memset(block, 0, count * size);
    return block;
}

/*
 * kfree -  frees the memory space pointed to by ptr, which must have been
 * called by a previous call to malloc.
 */
void kfree(void* ptr) {
    using namespace heapHelpers;
    LockGuardP g{theLock};
    // assert(ptr != nullptr);                        // check for null pointer
    memory_block_t* freeBlock = get_block(ptr);  // block we want to add back to free list
    K::memset(ptr, 0, freeBlock->block_size_alloc & ~0x1);
    deallocate(freeBlock);
    if (free_head == nullptr)  // list is empty, make block the head
    {
        free_head = freeBlock;
    } else if (freeBlock < free_head) {  // block needs to be head
        freeBlock->next = free_head;
        free_head = freeBlock;
    } else  // find spot in list for the block
    {
        memory_block_t* cur = free_head;
        while (cur->next != nullptr && freeBlock > cur) {
            if (freeBlock < cur->next) {
                freeBlock->next = cur->next;
                cur->next = freeBlock;
                return;
            }
            cur = cur->next;
        }  // add free block to end of the list
        cur->next = freeBlock;
    }
    return;
}

/*****************/
/* C++ operators */
/*****************/
// typedef long unsigned int size_t;

void* operator new(size_t size) {
    void* p = kmalloc(size);
    // if (p == 0) Debug::panic("out of memory");
    return p;
}

void operator delete(void* p) noexcept {
    if (!p) return;
    return kfree(p);
}

void operator delete(void* p, size_t sz) {
    if (!p) return;
    return kfree(p);
}

void* operator new[](size_t size) {
    void* p = kmalloc(size);
    // if (p == 0) Debug::panic("out of memory");
    return p;
}

void operator delete[](void* p) noexcept {
    if (!p) return;
    return kfree(p);
}

void operator delete[](void* p, size_t sz) {
    if (!p) return;
    return kfree(p);
}