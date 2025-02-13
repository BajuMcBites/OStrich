#include "vm.h"
#include "heap.h"
#include "stdint.h"
#include "libk.h"

uint64_t PGD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PUD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PMD[512] __attribute__((aligned(4096), section(".paging")));

void patch_page_tables() {

    for (int i = 504; i < 512; i++) {
        PMD[i] = PMD[i] & (0xFFFFFFFFFFFFF000);
        PMD[i] = PMD[i] | DEVICE_LOWER_ATTRIBUTES;
    }
}

void PageTable::map_vaddr(uint64_t vaddr, uint64_t paddr) {
    
    uint64_t pgd_index = get_pgd_index(vaddr);
    uint64_t pgd_descripter = PGD.descripters[pgd_index];

    uintptr_t pud_ptr = descriptor_to_vaddr(pgd_descripter);
    if (pud_ptr == nullptr) {
        PUDTable* pud_table = new PUDTable;

    }

    



}

uint64_t pgd_index(uint64_t vaddr) {
    return (vaddr >> 38) && 0x1FF;
}

uint64_t pud_index(uint64_t vaddr) {
    return (vaddr >> 29) && 0x1FF;
}

uint64_t pmd_index(uint64_t vaddr) {
    return (vaddr >> 20) && 0x1FF;

}

uint64_t pte_index(uint64_t vaddr) {
    return (vaddr >> 11) && 0x1FF;
}