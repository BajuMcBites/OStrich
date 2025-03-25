#ifndef _MMAP_H
#define _MMAP_H

#include "event.h"
#include "function.h"
#include "vm.h"

enum MMAPFlag { MAP_SHARED, MAP_PRIVATE, MAP_ANONYMOUS, MAP_NORESERVE };

enum MMAPProt { PROT_EXEC, PROT_READ, PROT_WRITE, PROT_NONE };

void load_mmapped_page(UserTCB* tcb, uint64_t uvaddr, Function<void(uint64_t)> w);
void mmap(UserTCB* tcb, uint64_t uvaddr, MMAPProt prot, MMAPFlag flags, char* file_name,
          uint64_t offset, int length, Function<void(void)> w);
void mmap_page(UserTCB* tcb, uint64_t uvaddr, MMAPProt prot, MMAPFlag flags, char* file_name,
               uint64_t offset, Function<void(void)> w);
void load_location(PageLocation* location, Function<void(uint64_t)> w);

#endif