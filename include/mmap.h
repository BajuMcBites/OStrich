#ifndef _MMAP_H
#define _MMAP_H

#include "event.h"
#include "function.h"
#include "vm.h"

#define PROT_NONE 0x0
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4

/**
 * Flags:
 * bits [1:0] shared, shared validate, private
 *
 * bits [31:2] other flags ored in
 */

#define MAP_SHARED 0x0
#define MAP_SHARED_VALIDATE 0x1  // dont use right now
#define MAP_PRIVATE 0x2

#define MAP_ANONYMOUS 0x4
#define MAP_NORESERVE 0x8

void load_mmapped_page(UserTCB* tcb, uint64_t uvaddr, Function<void(uint64_t)> w);
void mmap(UserTCB* tcb, uint64_t uvaddr, int prot, int flags, char* file_name, uint64_t offset,
          int length, Function<void(void)> w);
void mmap_page(UserTCB* tcb, uint64_t uvaddr, int prot, int flags, char* file_name, uint64_t offset,
               Function<void(void)> w);
void load_location(PageLocation* location, Function<void(uint64_t)> w);

#endif