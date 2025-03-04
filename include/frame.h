#ifndef _FRAME_H
#define _FRAME_H

#include "event.h"
#include "function.h"
#include "stdint.h"
#include "threads.h"
#include "vm.h"

#define USED_PAGE_FLAG 0x1
#define PINNED_PAGE_FLAG 0x2

void create_frame_table(uintptr_t start, int size);

void alloc_frame2(int flags, Function<void(int)> w);

void alloc_frame(int flags, Function<void(uint64_t)> w);

bool free_frame(uintptr_t frame_addr);

void pin_frame(uintptr_t frame_addr);

void unpin_frame(uintptr_t frame_addr);

typedef struct Frame {
    int flags;
} Frame;

#endif