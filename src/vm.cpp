#include "vm.h"
#include "stdint.h"

uint64_t PGD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PUD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PMD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PTE[512] __attribute__((aligned(4096), section(".paging")));
static unsigned short mem_map [ PAGING_PAGES ] = {0,};

void patch_page_tables() {

    for (int i = 504; i < 512; i++) {
        PMD[i] = PMD[i] & (0xFFFFFFFFFFFFF000);
        PMD[i] = PMD[i] | DEVICE_LOWER_ATTRIBUTES;
    }
}

unsigned long get_free_page()
{
	for (int i = 0; i < PAGING_PAGES; i++){
		if (mem_map[i] == 0){
			mem_map[i] = 1;
			return LOW_MEMORY + i*PAGE_SIZE;
		}
	}
	return 0;
}

void free_page(unsigned long p){
	mem_map[(p - LOW_MEMORY) / PAGE_SIZE] = 0;
}