#include "vm.h"

#include "event.h"
#include "frame.h"
#include "fs.h"
#include "heap.h"
#include "libk.h"
#include "stdint.h"
#include "swap.h"

uint64_t PGD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PUD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PMD[512] __attribute__((aligned(4096), section(".paging")));

PageCache* page_cache;
Swap* swap;

/**
 * used for setting up kernel memory for devices
 */
void patch_page_tables() {
    for (int i = 504; i < 512; i++) {
        PMD[i] = PMD[i] & (~0xFFF);
        PMD[i] = PMD[i] | DEVICE_LOWER_ATTRIBUTES;
    }
}

PageTable::PageTable() {
    this->pgd = nullptr;
}

/**
 * recursively unpins and frees every page for the associated page table
 */
PageTable::~PageTable() {
    if (this->pgd == nullptr) {
        return;
    }
    uint64_t pgd_paddr = vaddr_to_paddr((uint64_t)this->pgd);
    free_pgd();
    unpin_frame(pgd_paddr);
    free_frame(pgd_paddr);
}

void PageTable::free_pgd() {
    uint64_t descriptor;
    pud_t* pud;
    uint64_t pud_paddr;
    for (int i = 0; i < TABLE_ENTRIES; i++) {
        descriptor = this->pgd->descriptors[i];
        pud = descriptor_to_vaddr(descriptor);
        if (pud != nullptr) {
            free_pud(pud);
            pud_paddr = vaddr_to_paddr((uint64_t)pud);
            unpin_frame(pud_paddr);
            free_frame(pud_paddr);
        }
    }
}

void PageTable::free_pud(pud_t* pud) {
    uint64_t descriptor;
    pmd_t* pmd;
    uint64_t pmd_paddr;
    for (int i = 0; i < TABLE_ENTRIES; i++) {
        descriptor = pud->descriptors[i];
        pmd = descriptor_to_vaddr(descriptor);
        if (pmd != nullptr) {
            free_pmd(pmd);
            pmd_paddr = vaddr_to_paddr((uint64_t)pmd);
            unpin_frame(pmd_paddr);
            free_frame(pmd_paddr);
        }
    }
}

void PageTable::free_pmd(pmd_t* pmd) {
    uint64_t descriptor;
    pte_t* pte;
    uint64_t pte_paddr;
    for (int i = 0; i < TABLE_ENTRIES; i++) {
        descriptor = pmd->descriptors[i];
        pte = descriptor_to_vaddr(descriptor);
        if (pte != nullptr) {
            free_pte(pte);
            pte_paddr = vaddr_to_paddr((uint64_t)pte);
            unpin_frame(pte_paddr);
            free_frame(pte_paddr);
        }
    }
}

void PageTable::free_pte(pte_t* pte) {
    /* do nothing rn, maybe free mapped pages, but not too sure tbh*/
}

void PageTable::use_page_table() {
    uint64_t pgd_paddr = vaddr_to_paddr((uint64_t)this->pgd);
    switch_ttbr0(pgd_paddr);
}

void PageTable::map_vaddr(uint64_t vaddr, uint64_t paddr, uint64_t page_attributes,
                          Function<void()> w) {
    K::assert((paddr & 0xFFF) == 0, "non-aligned paddr for va to pa mapping");
    K::assert((vaddr & 0xFFF) == 0, "non-aligned vaddr for va to pa mappin");
    if (this->pgd == nullptr) {
        alloc_pgd([this, vaddr, paddr, page_attributes, w]() {
            map_vaddr_pgd(vaddr, paddr, page_attributes, w);
        });
    } else {
        map_vaddr_pgd(vaddr, paddr, page_attributes, w);
    }
}

void PageTable::alloc_pgd(Function<void()> w) {
    alloc_frame(PINNED_PAGE_FLAG, [this, w](uint64_t paddr) {
        this->pgd = (pgd_t*)paddr_to_vaddr(paddr);
        K::memset((void*)this->pgd, 0, PAGE_SIZE);
        K::assert(this->pgd != nullptr, "palloc failed");
        create_event(w, 1);
    });
}

void PageTable::map_vaddr_pgd(uint64_t vaddr, uint64_t paddr, uint64_t page_attributes,
                              Function<void()> w) {
    uint64_t pgd_index = get_pgd_index(vaddr);
    uint64_t pgd_descriptor = pgd->descriptors[pgd_index];

    pud_t* pud = descriptor_to_vaddr(pgd_descriptor);
    if (pud == nullptr) {
        alloc_frame(PINNED_PAGE_FLAG, [=](uint64_t pud_paddr) {
            K::assert(pud_paddr != nullptr, "palloc failed");
            pgd->descriptors[pgd_index] =
                paddr_to_table_descriptor(pud_paddr, page_attributes);  // put frame into table
            pud_t* pud = (pud_t*)paddr_to_vaddr(pud_paddr);             // get vaddr of frame
            K::memset((void*)pud, 0, PAGE_SIZE);
            map_vaddr_pud(pud, vaddr, paddr, page_attributes, w);
        });
    } else {
        map_vaddr_pud(pud, vaddr, paddr, page_attributes, w);
    }
}

void PageTable::map_vaddr_pud(pud_t* pud, uint64_t vaddr, uint64_t paddr, uint64_t page_attributes,
                              Function<void()> w) {
    uint64_t pud_index = get_pud_index(vaddr);
    uint64_t pud_descriptor = pud->descriptors[pud_index];

    pmd_t* pmd = descriptor_to_vaddr(pud_descriptor);
    if (pmd == nullptr) {
        alloc_frame(PINNED_PAGE_FLAG, [=](uint64_t pmd_paddr) {
            K::assert(pmd_paddr != nullptr, "palloc failed");
            pud->descriptors[pud_index] = paddr_to_table_descriptor(pmd_paddr, page_attributes);
            pmd_t* pmd = (pmd_t*)paddr_to_vaddr(pmd_paddr);
            K::memset((void*)pmd, 0, PAGE_SIZE);
            map_vaddr_pmd(pmd, vaddr, paddr, page_attributes, w);
        });
    } else {
        map_vaddr_pmd(pmd, vaddr, paddr, page_attributes, w);
    }
}

void PageTable::map_vaddr_pmd(pud_t* pmd, uint64_t vaddr, uint64_t paddr, uint64_t page_attributes,
                              Function<void()> w) {
    uint64_t pmd_index = get_pmd_index(vaddr);
    uint64_t pmd_descriptor = pmd->descriptors[pmd_index];

    pte_t* pte = descriptor_to_vaddr(pmd_descriptor);
    if (pte == nullptr) {
        alloc_frame(PINNED_PAGE_FLAG, [=](uint64_t pte_paddr) {
            K::assert(pte_paddr != nullptr, "palloc failed");
            pmd->descriptors[pmd_index] = paddr_to_table_descriptor(pte_paddr, page_attributes);
            pte_t* pte = (pte_t*)paddr_to_vaddr(pte_paddr);
            K::memset((void*)pte, 0, PAGE_SIZE);
            map_vaddr_pte(pte, vaddr, paddr, page_attributes);
            create_event(w, 1);
        });
    } else {
        map_vaddr_pte(pte, vaddr, paddr, page_attributes);
        create_event(w, 1);
    }
}

void PageTable::map_vaddr_pte(pud_t* pte, uint64_t vaddr, uint64_t paddr,
                              uint64_t page_attributes) {
    uint64_t pte_index = get_pte_index(vaddr);
    pte->descriptors[pte_index] = paddr_to_block_descriptor(paddr, page_attributes);
}

/**
 * returns false if the vaddr was not mapped in the first place
 * returns true if the vaddr was mapped and is now unmapped
 */
bool PageTable::unmap_vaddr(uint64_t vaddr) {
    if (pgd == nullptr) {
        return false;
    }

    uint64_t pgd_index = get_pgd_index(vaddr);
    uint64_t pgd_descriptor = pgd->descriptors[pgd_index];
    PageTableLevel* pud = descriptor_to_vaddr(pgd_descriptor);

    if (pud == nullptr) {
        return false;
    }

    uint64_t pud_index = get_pud_index(vaddr);
    uint64_t pud_descriptor = pud->descriptors[pud_index];
    PageTableLevel* pmd = descriptor_to_vaddr(pud_descriptor);

    if (pmd == nullptr) {
        return false;
    }

    uint64_t pmd_index = get_pmd_index(vaddr);
    uint64_t pmd_descriptor = pmd->descriptors[pmd_index];
    PageTableLevel* pte = descriptor_to_vaddr(pmd_descriptor);

    if (pte == nullptr) {
        return false;
    }

    uint64_t pte_index = get_pte_index(vaddr);
    pte->descriptors[pte_index] = 0;

    if ((pte->descriptors[pte_index] & 0x1) != 0) {
        pte->descriptors[pte_index] = 0;
        return true;
    }

    return false;
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

uint64_t paddr_to_table_descriptor(uint64_t paddr, uint64_t page_attributes) {
    K::assert((paddr & 0xFFF) == 0, "non-aligned paddr for table descriptor");
    K::assert(paddr != 0, "null paddr for table descriptor");
    uint64_t descriptor = paddr & ~0xFFF & ~((uint64_t)0xFF << 48);
    descriptor |= (TABLE_ENTRY | VALID_DESCRIPTOR);
    // descriptor |= (page_attributes);
    return descriptor;
}

uint64_t paddr_to_block_descriptor(uint64_t paddr, uint64_t page_attributes) {
    K::assert((paddr & 0xFFF) == 0, "non-aligned paddr for block descriptor");
    K::assert(paddr != 0, "null paddr for block descriptor");
    uint64_t descriptor = paddr & ~0xFFF & ~((uint64_t)0xFF << 48);
    descriptor |= (PAGE_ENTRY | VALID_DESCRIPTOR);
    descriptor |= (page_attributes);
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

LocalPageLocation* SupplementalPageTable::vaddr_mapping(uint64_t vaddr) {
    return this->map.get(vaddr);
}

void SupplementalPageTable::map_vaddr(uint64_t vaddr, LocalPageLocation* local) {
    this->map.put(vaddr, local);
}

/**
 * Assume all locks are appropriately held to call this. Returns bits [11:2] and
 * [63:52] of a 4096 block descriptor for a user page. Bits [51:12] must be
 * returned 0.
 */
uint64_t build_page_attributes(LocalPageLocation* local) {
    uint64_t attribute = 0;

    if ((local->perm & EXEC_PERM) == 0) {
        attribute |= (0x1L << 54);  // set XN (execute never) to true
    }

    if (local->sharing_mode == PRIVATE && local->location->ref_count > 1) {
        attribute |= (0x3L << 6);

    } else {
        if ((local->perm & WRITE_PERM) && (local->perm & EXEC_PERM) == 0) {
            attribute |= (0x1L << 6);
        } else if ((local->perm & WRITE_PERM) && (local->perm & EXEC_PERM) != 0) {
            attribute |= (0x01L << 6);
        } else {
            attribute |= (0x01L << 6);
        }
    }

    attribute |= (0x2L << 2);   // 2nd index in mair
    attribute |= (0x1L << 11);  // set nG (non global) [11] bit to true
    attribute |= (0x1L << 10);  // set AF (access flag) [10] bit to true so we dont fault on access
    attribute |= (0x3L << 8);   // set sharability [9:8] to inner sharable across cores

    return attribute;
}

void add_local(PageLocation* location, LocalPageLocation* local) {
    location->ref_count++;

    if (location->users == nullptr) {
        location->users = local;
        local->prev = nullptr;
        local->next = nullptr;
        return;
    }

    LocalPageLocation* n = location->users;
    int count = 2;

    while (count < location->ref_count) {
        count++;
        n = n->next;
    }

    n->next = local;
    local->prev = n;
}

void remove_local(PageLocation* location, LocalPageLocation* local) {
    location->ref_count--;

    LocalPageLocation* prev = local->prev;
    LocalPageLocation* next = local->next;

    if (prev == nullptr) {
        location->users = next;
    } else {
        prev->next = next;
    }

    if (next != nullptr) {
        next->prev = prev;
    }
}

void unmap_refs(PageLocation* location) {
    LocalPageLocation* n = location->users;

    while (n != nullptr) {
        // if (n->sharing_mode == PRIVATE) {
            n->pcb->page_table->unmap_vaddr(n->uvaddr);
        // }

        n = n->next;
    }
}

/**
 * gets or adds a page location from/to the page cache
 *
 * takes in file name offset and id and finds the matching page in the page or
 * creates a new one and inserts it into the page cache.
 */
void PageCache::get_or_add(file* file, uint64_t offset, uint64_t id, LocalPageLocation* local,
                           Function<void(PageLocation*)> w) {
    bool file_backed = file != nullptr;
    bool unbacked = offset == 1;
    lock.lock([=]() {
        PCKey key(file, offset, id);
        PageLocation* location = map.get(key);

        if (location == nullptr) {
            location = new PageLocation;
            location->ref_count = 0;
            location->present = false;
            location->users = nullptr;

            if (file_backed) {
                location->location_type = FILESYSTEM;
                location->location.filesystem = new FileLocation(file, offset);
            } else {
                if (unbacked) {
                    location->location_type = UNBACKED;
                    location->location.swap = new SwapLocation(id);
                } else {
                    location->location_type = SWAP;
                    location->location.swap = new SwapLocation(id);
                }
            }
            map.put(key, location);
        }
        add_local(location, local);
        local->location = location;
        create_event(w, location);
        lock.unlock();
    });
}

void PageCache::remove(LocalPageLocation* local, Function<void(void)> w) {
    lock.lock([=]() {
        PageLocation* location = local->location;

        remove_local(location, local);

        if (location->ref_count == 0) {
            if (location->location_type == FILESYSTEM) {
                map.remove(PCKey(location->location.filesystem->file,
                                 location->location.filesystem->offset, 0));
            } else if (location->location_type == UNBACKED) {
                map.remove(PCKey(nullptr, 1, location->location.swap->swap_id));
            } else if (location->location_type == SWAP) {
                map.remove(PCKey(nullptr, 0, location->location.swap->swap_id));
            }

            delete location;
        } else {
            local->location->lock.unlock();
        }
        lock.unlock();
        create_event(w);
    });
}

void init_page_cache() {
    page_cache = new PageCache;
}

void init_swap() {
    swap = new Swap(4096);
}

PageLocation::~PageLocation() {
    if (location_type == FILESYSTEM) {
        delete location.filesystem;
    } else if (location_type == SWAP) {
        delete location.swap;
    }

    if (present) {
        unpin_frame(paddr);
        free_frame(paddr);
        /* todo: send signal to swap that the space is no longer needed */
    }
}

void SupplementalPageTable::copy_mappings(SupplementalPageTable* other, PCB* pcb,
                                          Function<void(void)> w) {
    other->lock.lock([=]() {
        this->lock.lock([=]() {
            Semaphore* sema = new Semaphore((-other->map.size) + 1);
            other->map.for_each([=](LocalPageLocation* local) {
                PageLocation* location = local->location;
                location->lock.lock([=]() {
                    page_cache->lock.lock([=]() {
                        LocalPageLocation* new_local = new LocalPageLocation(
                            pcb, local->perm, local->sharing_mode, local->uvaddr);
                        add_local(location, new_local);
                        unmap_refs(location);

                        new_local->location = location;

                        this->map.put(local->uvaddr, new_local);

                        page_cache->lock.unlock();
                        location->lock.unlock();

                        sema->up();
                    });
                });
            });

            sema->down([=]() {
                delete sema;
                this->lock.unlock();
                other->lock.unlock();
                create_event(w);
            });
        });
    });
}
