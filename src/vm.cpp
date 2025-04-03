#include "vm.h"

#include "frame.h"
#include "heap.h"
#include "libk.h"
#include "printf.h"
#include "stdint.h"

uint64_t PGD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PUD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PMD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PMD_arm[512] __attribute__((aligned(4096), section(".paging")));

void getMeFlag() {
    printf("pmd[0] 0x%x\n", PMD[0]);
}
/**
 * used for setting up kernel memory for devices
 */
void patch_page_tables() {
    for (int i = 504; i < 512; i++) {
        PMD[i] = PMD[i] & (~0xFFF);
        PMD[i] = PMD[i] | DEVICE_LOWER_ATTRIBUTES;
    }
    // printf("pmd[0] 0x%x\n", PMD[0]);
    PUD[1] = PUD[1] & (~0xFFF);
    PUD[1] = PUD[1] | 0x401;

    for (int i = 0; i < 8; i++) {
        PMD_arm[i] = PMD_arm[i] & (~0xFFF);
        PMD_arm[i] = PMD_arm[i] | DEVICE_LOWER_ATTRIBUTES;
    }
}

PageTable::PageTable(Function<void()> w) {
    alloc_frame(PINNED_PAGE_FLAG, [this, w](uint64_t paddr) {
        this->pgd = (pgd_t*)paddr_to_vaddr(paddr);
        K::assert(this->pgd != nullptr, "palloc failed");
        create_event(w, 1);
    });
}

void PageTable::use_page_table() {
    uint64_t pgd_paddr = vaddr_to_paddr((uint64_t)this->pgd);
    switch_ttbr0(pgd_paddr);
}

void PageTable::map_vaddr(uint64_t vaddr, uint64_t paddr, uint16_t lower_attributes,
                          Function<void()> w) {
    K::assert((paddr & 0xFFF) == 0, "non-aligned paddr for va to pa mapping");
    K::assert((vaddr & 0xFFF) == 0, "non-aligned vaddr for va to pa mappin");
    map_vaddr_pgd(vaddr, paddr, lower_attributes, w);
}

void PageTable::map_vaddr_pgd(uint64_t vaddr, uint64_t paddr, uint16_t lower_attributes,
                              Function<void()> w) {
    uint64_t pgd_index = get_pgd_index(vaddr);
    uint64_t pgd_descriptor = pgd->descriptors[pgd_index];

    pud_t* pud = descriptor_to_vaddr(pgd_descriptor);
    if (pud == nullptr) {
        alloc_frame(PINNED_PAGE_FLAG, [=](uint64_t pud_paddr) {
            K::assert(pud_paddr != nullptr, "palloc failed");
            pgd->descriptors[pgd_index] =
                paddr_to_table_descriptor(pud_paddr);        // put frame into table
            pud_t* pud = (pud_t*)paddr_to_vaddr(pud_paddr);  // get vaddr of frame
            map_vaddr_pud(pud, vaddr, paddr, lower_attributes, w);
        });
    } else {
        map_vaddr_pud(pud, vaddr, paddr, lower_attributes, w);
    }
}

void PageTable::map_vaddr_pud(pud_t* pud, uint64_t vaddr, uint64_t paddr, uint16_t lower_attributes,
                              Function<void()> w) {
    uint64_t pud_index = get_pud_index(vaddr);
    uint64_t pud_descriptor = pud->descriptors[pud_index];

    pmd_t* pmd = descriptor_to_vaddr(pud_descriptor);
    if (pmd == nullptr) {
        alloc_frame(PINNED_PAGE_FLAG, [=](uint64_t pmd_paddr) {
            K::assert(pmd_paddr != nullptr, "palloc failed");
            pud->descriptors[pud_index] = paddr_to_table_descriptor(pmd_paddr);
            pmd_t* pmd = (pmd_t*)paddr_to_vaddr(pmd_paddr);
            map_vaddr_pmd(pmd, vaddr, paddr, lower_attributes, w);
        });
    } else {
        map_vaddr_pmd(pmd, vaddr, paddr, lower_attributes, w);
    }
}

void PageTable::map_vaddr_pmd(pud_t* pmd, uint64_t vaddr, uint64_t paddr, uint16_t lower_attributes,
                              Function<void()> w) {
    uint64_t pmd_index = get_pmd_index(vaddr);
    uint64_t pmd_descriptor = pmd->descriptors[pmd_index];

    pte_t* pte = descriptor_to_vaddr(pmd_descriptor);
    if (pte == nullptr) {
        alloc_frame(PINNED_PAGE_FLAG, [=](uint64_t pte_paddr) {
            K::assert(pte_paddr != nullptr, "palloc failed");
            pmd->descriptors[pmd_index] = paddr_to_table_descriptor(pte_paddr);
            pte_t* pte = (pte_t*)paddr_to_vaddr(pte_paddr);
            map_vaddr_pte(pte, vaddr, paddr, lower_attributes);
            create_event(w, 1);
        });
    } else {
        map_vaddr_pte(pte, vaddr, paddr, lower_attributes);
        create_event(w, 1);
    }
}

void PageTable::map_vaddr_pte(pud_t* pte, uint64_t vaddr, uint64_t paddr,
                              uint16_t lower_attributes) {
    uint64_t pte_index = get_pte_index(vaddr);
    pte->descriptors[pte_index] = paddr_to_block_descriptor(paddr, lower_attributes);
}

uint64_t get_pgd_index(uint64_t vaddr) {
    return (vaddr >> 39) & 0x1FF;
}

uint64_t get_pud_index(uint64_t vaddr) {
    return (vaddr >> 30) & 0x1FF;
}

uint64_t get_pmd_index(uint64_t vaddr) {
    return (vaddr >> 21) & 0x1FF;
}

uint64_t get_pte_index(uint64_t vaddr) {
    return (vaddr >> 12) & 0x1FF;
}

/**
 * vaddr being kernel virtual address
 */
uint64_t paddr_to_vaddr(uint64_t paddr) {
    return paddr | VA_START;
}

/**
 * vaddr being kernel virtual address
 */
uint64_t vaddr_to_paddr(uint64_t vaddr) {
    return vaddr & (~VA_START);
}

uint64_t paddr_to_table_descriptor(uint64_t paddr) {
    K::assert((paddr & 0xFFF) == 0, "non-aligned paddr for table descriptor");
    K::assert(paddr != 0, "null paddr for table descriptor");
    uint64_t descriptor = paddr & ~0xFFF & ~((uint64_t)0xFF << 48);
    descriptor |= (TABLE_ENTRY | VALID_DESCRIPTOR);
    return descriptor;
}

uint64_t paddr_to_block_descriptor(uint64_t paddr, uint16_t lower_attributes) {
    K::assert((paddr & 0xFFF) == 0, "non-aligned paddr for block descriptor");
    K::assert(paddr != 0, "null paddr for block descriptor");
    uint64_t descriptor = paddr & ~0xFFF & ~((uint64_t)0xFF << 48);
    descriptor |= (PAGE_ENTRY | VALID_DESCRIPTOR);
    descriptor |= lower_attributes;
    return descriptor;
}

/**
 * returns 0 for any non valid descriptor
 */
uint64_t descriptor_to_paddr(uint64_t descriptor) {
    if ((descriptor & 0x1) == 0) {
        return 0;
    }
    return descriptor & (~0xFFF);
}

PageTableLevel* descriptor_to_vaddr(uint64_t descriptor) {
    uint64_t paddr = descriptor_to_paddr(descriptor);
    if (paddr == 0) {
        return nullptr;
    }
    return (PageTableLevel*)paddr_to_vaddr(paddr);
}
