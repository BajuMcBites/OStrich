#include "frame.h"

#include "atomic.h"
#include "event.h"
#include "printf.h"
#include "stdint.h"
#include "vm.h"
#include "mmap.h"

uint64_t index = 0;
uint64_t eviction_index = 0;
uint64_t num_frames = 0;
Frame* frame_table = 0;
SpinLock lock;  // switch to blocking lock

void create_frame_table(uintptr_t start, int size) {
    frame_table = (Frame*)start;
    num_frames = size / PAGE_SIZE;
    for (int i = 0; i < num_frames; i++) {
        frame_table[i].flags = 0;
        frame_table[i].contents = nullptr;
        frame_table[i].pin_count = 0;
    }

    for (int i = 0; i < (int)((start / PAGE_SIZE) + num_frames * sizeof(Frame) / PAGE_SIZE + 1);
         i++) {
        frame_table[i].flags |= USED_PAGE_FLAG;
        frame_table[i].flags |= PINNED_PAGE_FLAG;
        frame_table[i].pin_count = 1;
    }
    // Pin device memory, assume MMIO is the last 16 MB, 0x1000000 (Raspberry Pi
    // 3b)
    int mmio_start_addr = size - 0x1000000;
    for (int i = mmio_start_addr / PAGE_SIZE; i < num_frames; i++) {
        frame_table[i].flags |= USED_PAGE_FLAG;
        frame_table[i].flags |= PINNED_PAGE_FLAG;
        frame_table[i].pin_count = 1;
    }
}
int get_free_index_unlocked() {
    for (int i = 0; i < num_frames; i++) {
        if (!(frame_table[index].flags & USED_PAGE_FLAG)) {
            int temp = index;
            index += 1;
            index %= num_frames;
            return temp;
        }
        index += 1;
        index %= num_frames;
    }
    // no free frames
    return -1;
}

/*
 * Evicts a frame from frame table
 */
void evict_frame(Function<void(uint64_t)> w) {
    while ((frame_table[eviction_index].flags & USED_PAGE_FLAG) || frame_table[eviction_index].pin_count > 0 
        || frame_table[eviction_index].contents == nullptr)
        eviction_index = (eviction_index + 1) % num_frames;

    
    write_location(frame_table[eviction_index].contents, [=]() {
        lock.unlock();
        create_event(w, (uint64_t) (eviction_index * PAGE_SIZE));
    });
}

/**
 * allocates a frame from physical memory
 */
void alloc_frame(int flags, Function<void(uint64_t)> w) {
    alloc_frame(flags, nullptr, w);
}

/**
 * allocates a frame from physical memory + maps location struct
 */
void alloc_frame(int flags, PageLocation* location, Function<void(uint64_t)> w) {
    lock.lock();
    int index = get_free_index_unlocked();
    if (index != -1) {
        frame_table[index].flags = flags;
        frame_table[index].flags |= USED_PAGE_FLAG;
        frame_table[index].contents = location;
        create_event<uint64_t>(w, index * PAGE_SIZE, 1);
        lock.unlock();
    } else {
        evict_frame(w);
    }
}

bool free_frame(uintptr_t frame_addr) {
    LockGuard<SpinLock>{lock};
    int index = frame_addr / PAGE_SIZE;
    if (index >= 0 && index < num_frames && !(frame_table[index].flags & PINNED_PAGE_FLAG)) {
        frame_table[index].flags &= ~USED_PAGE_FLAG;
        return true;
    }
    return false;
}

void pin_frame(uintptr_t frame_addr) {
    LockGuard<SpinLock>{lock};
    int index = frame_addr / PAGE_SIZE;
    if (index >= 0 && index < num_frames) {
        frame_table[index].flags |= PINNED_PAGE_FLAG;
        frame_table[index].pin_count += 1;
    }
}

void unpin_frame(uintptr_t frame_addr) {
    LockGuard<SpinLock>{lock};
    int index = frame_addr / PAGE_SIZE;
    if (index >= 0 && index < num_frames) {
        if (frame_table[index].pin_count >= 1) frame_table[index].pin_count -= 1;

        if (frame_table[index].pin_count == 0) frame_table[index].flags &= ~PINNED_PAGE_FLAG;
    }
}