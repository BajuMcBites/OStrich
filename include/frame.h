#ifndef _FRAME_H
#define _FRAME_H

#include "vm.h"
#include "stdint.h"
#include "event.h"
#include "function.h"
#include "threads.h"

#define USED_PAGE_FLAG 0x1
#define PINNED_PAGE_FLAG 0x2

void create_frame_table(uintptr_t start, int size);

template <typename T>
void alloc_frame2(int flags, Function<void(int)> w);

void alloc_frame(int flags, Function<void(int)> w);

bool free_frame(uintptr_t frame_addr);

void pin_frame(uintptr_t frame_addr);

void unpin_frame(uintptr_t frame_addr);

typedef struct Frame
{
    int flags;
} Frame;

#endif