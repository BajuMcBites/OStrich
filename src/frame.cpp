#include "frame.h"
#include "event_loop.h"
#include "printf.h"
#include "threads.h"
#include "stdint.h"
#include "vm.h"

int index = 0;
int num_frames = 0;
Frame* frame_table = 0;
void create_frame_table(uintptr_t start, int size) {
    frame_table = (Frame*)start;
    num_frames = size / PAGE_SIZE;
    for (int i = 0; i < num_frames; i++)
    {
        frame_table[i].flags = 0;
    }

    for (int i = 0; i < (int)((start / PAGE_SIZE) + num_frames * sizeof(Frame) / PAGE_SIZE + 1);
         i++) {
        frame_table[i].flags |= USED_PAGE_FLAG;
        frame_table[i].flags |= PINNED_PAGE_FLAG;
    }
    // Pin device memory, assume MMIO is the last 16 MB, 0x1000000 (Raspberry Pi
    // 3b)
    int mmio_start_addr = size - 0x1000000;
    for (int i = mmio_start_addr / PAGE_SIZE; i < num_frames; i++)
    {
        frame_table[i].flags |= USED_PAGE_FLAG;
        frame_table[i].flags |= PINNED_PAGE_FLAG;
    }
}

void alloc_frame(int flags, Function<void(uint64_t)> w) {
    for (int i = 0; i < num_frames; i++) {
        if (!(frame_table[index].flags & USED_PAGE_FLAG)) {
            frame_table[index].flags = flags;
            frame_table[index].flags |= USED_PAGE_FLAG;
            create_event_value<uint64_t>(w, index * PAGE_SIZE, 1);
            index += 1;
            return;
        }
        index += 1;
        index %= num_frames;
    }
    // Need to evict
    return;
}

// template <typename T>
void alloc_frame2(int flags, Function<void(int)> w)
{
    for (int i = 0; i < num_frames; i++)
    {
        if (!(frame_table[index].flags & USED_PAGE_FLAG))
        {
            frame_table[index].flags = flags;
            frame_table[index].flags |= USED_PAGE_FLAG;
            create_kernel_event<int>(w, index * PAGE_SIZE);
            index += 1;
            return;
        }
        index += 1;
        index %= num_frames;
    }
    // Need to evict
    return;
}

bool free_frame(uintptr_t frame_addr)
{
    int index = frame_addr / PAGE_SIZE;
    if (index >= 0 && index < num_frames && !(frame_table[index].flags & PINNED_PAGE_FLAG))
    {
        frame_table[index].flags &= ~USED_PAGE_FLAG;
        return true;
    }
    return false;
}

void pin_frame(uintptr_t frame_addr)
{
    int index = frame_addr / PAGE_SIZE;
    if (index >= 0 && index < num_frames)
    {
        frame_table[index].flags |= PINNED_PAGE_FLAG;
    }
}

void unpin_frame(uintptr_t frame_addr)
{
    int index = frame_addr / PAGE_SIZE;
    if (index >= 0 && index < num_frames)
    {
        frame_table[index].flags &= ~PINNED_PAGE_FLAG;
    }
}