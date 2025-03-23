#ifndef _HEAP_H_
#define _HEAP_H_

#define ALIGNMENT 16 /* The alignment of all payloads returned by umalloc */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

#include "stdint.h"
/*
 * memory_block_t - Represents a block of memory managed by the heap. The
 * struct can be left as is, or modified for your design.
 * In the current design bit0 is the allocated bit
 * bits 1-3 are unused.
 * and the remaining 60 bit represent the size.
 */
typedef struct memory_block_struct {
    size_t block_size_alloc;
    struct memory_block_struct *next;
} memory_block_t;

// extern void uinit(void *start, size_t bytes);
// extern "C" void *kmalloc(size_t size);
// extern "C" void kfree(void *ptr);

void uinit(void *start, size_t bytes);
void *kmalloc(size_t size);
void *kcalloc(unsigned char val, size_t size);

void kfree(void *ptr);

void *operator new(size_t size);
void operator delete(void *p) noexcept;

void operator delete(void *p, size_t sz);
void *operator new[](size_t size);
void operator delete[](void *p) noexcept;
void operator delete[](void *p, size_t sz);

#endif