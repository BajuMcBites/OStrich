#include "vm.h"
#include "heap.h"
#include "stdint.h"
#include "libk.h"

uint64_t PGD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PUD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PMD[512] __attribute__((aligned(4096), section(".paging")));

uint64_t palloc(uint16_t FLAGS) {
    return nullptr;
}

void patch_page_tables() {

    for (int i = 504; i < 512; i++) {
        PMD[i] = PMD[i] & (0xFFFFFFFFFFFFF000);
        PMD[i] = PMD[i] | DEVICE_LOWER_ATTRIBUTES;
    }
}

//TODO: if palloc failes, evict and palloc again(idk how to do this)
//does palloc handle evictions

PageTable::PageTable() {
    pgd = (pgd_t*) palloc(0x3);
    K::assert(pgd != nullptr, "palloc failed");
}


void PageTable::map_vaddr(uint64_t vaddr, uint64_t paddr) {
    map_vaddr_pgd(vaddr, paddr);
}

void PageTable::map_vaddr_pgd(uint64_t vaddr, uint64_t paddr) {
    uint64_t pgd_index = get_pgd_index(vaddr);
    uint64_t pgd_descripter = pgd->descripters[pgd_index];

    pud_t* pud = descriptor_to_vaddr(pgd_descripter);
    if (pud == nullptr) {
        uint64_t pud_paddr = palloc(0x5); //get physical frame
        K::assert(pud_paddr != nullptr, "palloc failed");
        pgd->descripters[pgd_index] = vaddr_to_table_descripter(pud_paddr); //put frame into table
        pud = (pud_t*) paddr_to_vaddr(pud_paddr); // get vaddr of frame
    }
    map_vaddr_pud(pud, vaddr, paddr);
}

void PageTable::map_vaddr_pud(pud_t* pud, uint64_t vaddr, uint64_t paddr) {
    uint64_t pud_index = get_pud_index(vaddr);
    uint64_t pud_descripter = pud->descripters[pud_index];

    pmd_t* pmd = descriptor_to_vaddr(pud_descripter);
    if (pmd == nullptr) {
        uint64_t pmd_paddr = palloc(0x5);
        K::assert(pmd_paddr != nullptr, "palloc failed");
        pud->descripters[pud_index] = vaddr_to_table_descripter(pmd_paddr); 
        pmd = (pmd_t*) paddr_to_vaddr(pmd_paddr);
    }
    map_vaddr_pmd(pmd, vaddr, paddr);
}

void PageTable::map_vaddr_pmd(pud_t* pmd, uint64_t vaddr, uint64_t paddr) {
    uint64_t pmd_index = get_pmd_index(vaddr);
    uint64_t pmd_descripter = pmd->descripters[pmd_index];

    pmd_t* pte = descriptor_to_vaddr(pmd_descripter);
    if (pmd == nullptr) {
        uint64_t pte_paddr = palloc(0x5);
        K::assert(pte_paddr != nullptr, "palloc failed");
        pmd->descripters[pmd_index] = vaddr_to_table_descripter(pte_paddr);
        pte = (pte_t*) paddr_to_vaddr(pte_paddr);
    }
    map_vaddr_pte(pte, vaddr, paddr);
}

void PageTable::map_vaddr_pte(pud_t* pte, uint64_t vaddr, uint64_t paddr) {
    uint64_t pte_index = get_pte_index(vaddr);
    pte->descripters[pte_index] = vaddr_to_block_descripter(paddr);
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