#include "vm.h"
#include "stdint.h"

uint64_t PGD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PUD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PMD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PTE[512] __attribute__((aligned(4096), section(".paging")));

void patch_page_tables() {
    uint64_t lower_attributes = 0x1 | (0x0 << 2) | (0x1 << 10) | (0x0 << 6) | (0x0 << 8);
    for (int i = 504; i < 512; i++) {
        PMD[i] = PMD[i] & (0xFFFFFFFFFFFFF000);
        PMD[i] = PMD[i] | lower_attributes;
    }
}
