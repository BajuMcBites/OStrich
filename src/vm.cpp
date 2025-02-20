#include "vm.h"
#include "heap.h"
#include "stdint.h"
#include "libk.h"
#include "event_loop.h"

uint64_t PGD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PUD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PMD[512] __attribute__((aligned(4096), section(".paging")));

#define PINNED 0x2
template <typename work>
uint64_t alloc_frame(uint16_t FLAGS, work work) {
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

template <typename work>
PageTable::PageTable(work w) {
    alloc_frame(PINNED, [&pgd](uint64_t paddr) {
        pgd = paddr_to_vaddr(paddr);
        K::assert(pgd != nullptr, "palloc failed");
        create_event(work, 1);
    });
}

template <typename work>
void PageTable::map_vaddr(uint64_t vaddr, uint64_t paddr, work w) {
    map_vaddr_pgd(vaddr, paddr, w);
}

template <typename work>
void PageTable::map_vaddr_pgd(uint64_t vaddr, uint64_t paddr, work w) {
    uint64_t pgd_index = get_pgd_index(vaddr);
    uint64_t pgd_descripter = pgd->descripters[pgd_index];

    pud_t* pud = descriptor_to_vaddr(pgd_descripter);
    if (pud == nullptr) {
        alloc_frame(PINNED, [=](uint64_t pud_paddr){
            K::assert(pud_paddr != nullptr, "palloc failed");
            pgd->descripters[pgd_index] = vaddr_to_table_descripter(pud_paddr); //put frame into table
            pud = (pud_t*) paddr_to_vaddr(pud_paddr); // get vaddr of frame
            map_vaddr_pud(pud, vaddr, paddr, w);
        });
    } else {
        map_vaddr_pud(pud, vaddr, paddr, w);
    }

}

template <typename work>
void PageTable::map_vaddr_pud(pud_t* pud, uint64_t vaddr, uint64_t paddr, work w) {
    uint64_t pud_index = get_pud_index(vaddr);
    uint64_t pud_descripter = pud->descripters[pud_index];

    pmd_t* pmd = descriptor_to_vaddr(pud_descripter);
    if (pmd == nullptr) {
         alloc_frame(PINNED, [=](uint64_t pmd_paddr) {
            K::assert(pmd_paddr != nullptr, "palloc failed");
            pud->descripters[pud_index] = vaddr_to_table_descripter(pmd_paddr); 
            pmd = (pmd_t*) paddr_to_vaddr(pmd_paddr);
            map_vaddr_pmd(pmd, vaddr, paddr, w);
         });    
    } else {
        map_vaddr_pmd(pmd, vaddr, paddr, w);
    }
}

template <typename work>
void PageTable::map_vaddr_pmd(pud_t* pmd, uint64_t vaddr, uint64_t paddr, work work) {
    uint64_t pmd_index = get_pmd_index(vaddr);
    uint64_t pmd_descripter = pmd->descripters[pmd_index];

    pmd_t* pte = descriptor_to_vaddr(pmd_descripter);
    if (pmd == nullptr) {
        palloc(PINNED, [=](uint64_t pte_paddr) {
            K::assert(pte_paddr != nullptr, "palloc failed");
            pmd->descripters[pmd_index] = vaddr_to_table_descripter(pte_paddr);
            pte = (pte_t*) paddr_to_vaddr(pte_paddr);
            map_vaddr_pte(pte, vaddr, paddr);
            create_event(w, 1);
        });
    } else {
        map_vaddr_pte(pte, vaddr, paddr);
        create_event(w, 1);
    }
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