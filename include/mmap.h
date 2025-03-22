#ifndef _MMAP_H
#define _MMAP_H

#include "event.h"
#include "vm.h"
#include "function.h"

enum MMAPFlag {
    FILESYSTEM_m,
    SWAP_m,
    ANONYMOUS_m
};

void load_mmapped_page(UserTCB* tcb, uint64_t uvaddr, Function<void(uint64_t)> w);
void mmap(UserTCB* tcb, uint64_t uvaddr, int prot, MMAPFlag flags, char* file_name, uint64_t offset, Function<void(void)> w);
void load_location(PageLocation* location, Function<void(uint64_t)> w);

#endif