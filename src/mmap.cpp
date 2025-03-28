#include "mmap.h"

#include "frame.h"
#include "function.h"

extern PageCache* page_cache;

static SpinLock unbacked_id_lock;
static volatile int unbacked_id = 1;

/**
 *  Loads a page location into memory add passes the paddr of the page to the
 *  continuation function, this page is pinned in memory and should not be able
 *  to be evicted
 *
 *  **NOTE
 *         the PageLocation lock should be held by the caller of the function
 *         and should be released by the continuation function
 */
void load_location(PageLocation* location, Function<void(uint64_t)> w) {
    K::assert(location != nullptr, "we are null location");
    K::assert(!location->present, "we are trying to load an already loaded page");

    alloc_frame(PINNED_PAGE_FLAG, location, [=](uint64_t paddr) {
        void* page_vaddr = (void*)paddr_to_vaddr(paddr);
        K::memset(page_vaddr, 0, PAGE_SIZE);  // dont give none zero memory

        if (location->location_type == UNBACKED) { /* not backed page */

            location->present = true;
            location->paddr = paddr;
            create_event(w, paddr);
            return;

        } else if (location->location_type == SWAP) { /* backed page*/

            K::assert(false, "swap location type not implemented");

        } else if (location->location_type == FILESYSTEM) {
            FileLocation* file_location = location->location.filesystem;
            read(file_location->file, file_location->offset, (char*)page_vaddr, PAGE_SIZE,
                 [=](int ret) {
                     K::assert(ret >= 0, "read failed");
                     location->paddr = paddr;
                     location->present = true;

                     create_event(w, paddr);
                 });

        } else {
            K::assert(false, "invalid location type");
        }
    });
    return;
}

// /**
//  * fills in necessary tcb datastructures with info for mmapping a SINGLE page and
//  * runs the continuation function. This does not load the page into memory, which
//  * can be done by load_mmapped_page.
//  */
// void mmap_page(UserTCB* tcb, uint64_t uvaddr, int prot, int flags, char* file_name, uint64_t offset,
//                Function<void(void)> w) {
//     bool file_mmap = (flags & MAP_ANONYMOUS) == 0;
//     bool no_reserve = (flags & MAP_NORESERVE) != 0;

//     K::assert(uvaddr % PAGE_SIZE == 0, "invalid user vaddr passed to mmap");

//     tcb->supp_page_table->lock.lock([=]() {
//         LocalPageLocation* local = tcb->supp_page_table->vaddr_mapping(uvaddr);
//         K::assert(local == nullptr, "remapping an already mapped address");

//         local = new LocalPageLocation;
//         local->tcb = tcb;
//         local->perm = prot;
//         local->next = nullptr;

//         if ((flags & 0x3) == MAP_SHARED) {
//             local->sharing_mode = SHARED;
//         } else if ((flags & 0x3) == MAP_PRIVATE) {
//             local->sharing_mode = PRIVATE;
//         }

//         PageLocation* location = nullptr;

//         /* TODO: insert page cache look for page (atomic get or create function), return with
//          * refcount -1 if newly created */

//         location = new PageLocation;
//         location->ref_count = -1; /* simulating getting new one from page cache*/

//         location->lock.lock([=]() {
//             if (location->paddr == 1) {
//                 local->location = location;
//                 location->present = false;
//                 location->paddr = 0;

//                 if (file_mmap) {
//                     K::assert(file_name != nullptr, "mmapping nullptr file");

//                     location->location_type = FILESYSTEM;
//                     location->location.filesystem = new FileLocation(file_name, offset);
//                     tcb->supp_page_table->map_vaddr(uvaddr, local);
//                     location->lock.unlock();
//                     tcb->supp_page_table->lock.unlock();
//                     create_event(w);
//                     return;
//                 } else if (!file_mmap) {
//                     if (!no_reserve) {
//                         K::assert(false, "swap mmap not implemented yet");

//                     } else {
//                         location->location_type = UNBACKED;
//                         tcb->supp_page_table->map_vaddr(uvaddr, local);
//                         location->lock.unlock();
//                         tcb->supp_page_table->lock.unlock();
//                         create_event(w);
//                         return;
//                     }
//                     return;
//                 }
//                 K::assert(false, "mmap: we should not get here");
//             } else {
//                 /* TODO add local to locations stuff and increase ref count + alter linked list*/
//                 K::assert(false, "no implemnetation for two pages mapped to same location");
//                 return;
//             }
//         });
//         return;
//     });
//     return;
// }

void create_local_mapping(UserTCB* tcb, uint64_t uvaddr, int prot, int flags, file* file, uint64_t offset, uint64_t id,
    Function<void(void)> w) {

    tcb->supp_page_table->lock.lock([=]() {
        LocalPageLocation* local = tcb->supp_page_table->vaddr_mapping(uvaddr);
        K::assert(local == nullptr, "remapping an already mapped address");

        local = new LocalPageLocation;
        local->tcb = tcb;
        local->perm = prot;

        if ((flags & 0x3) == MAP_SHARED) {
            local->sharing_mode = SHARED;
        } else if ((flags & 0x3) == MAP_PRIVATE) {
            local->sharing_mode = PRIVATE;
        }

        page_cache->get_or_add(file, offset, id, local, [=](PageLocation* location) {
            tcb->supp_page_table->map_vaddr(uvaddr, local);
            tcb->supp_page_table->lock.unlock();
            create_event(w);
        });
        return;
    });
    return;
}

/**
 * fills in necessary tcb datastructures with info for mmapping a SINGLE page and
 * runs the continuation function. This does not load the page into memory, which
 * can be done by load_mmapped_page. If we want to request a new id for a swap 
 * or unbacked page, make sure id is 0. If file is null, we use offset to 
 * decide whether it is swap or unbacked (0 for swap 1 for unbacked). All 
 * params passed must match the flags passed aswell.
 */
void mmap_page(UserTCB* tcb, uint64_t uvaddr, int prot, int flags, file* file, uint64_t offset, uint64_t id,
    Function<void(void)> w) {

    bool file_mmap = (flags & MAP_ANONYMOUS) == 0;
    bool no_reserve = (flags & MAP_NORESERVE) != 0;

    K::assert(uvaddr % PAGE_SIZE == 0, "invalid user vaddr passed to mmap");
    K::assert(!file_mmap || file_mmap != nullptr, "we are mmapping a null file");

    K::assert(file_mmap || (no_reserve && (offset == 1)) || (!no_reserve && (offset == 0)), "flags dont match other mmap params");

    if (file_mmap || id != 0) {
        create_local_mapping(tcb, uvaddr, prot, flags, file, offset, id, w);
    } else {
        if (no_reserve) {
            unbacked_id_lock.lock();
            id = unbacked_id++;
            unbacked_id_lock.unlock();
            create_local_mapping(tcb, uvaddr, prot, flags, file, offset, id, w);
            return;
        } else{
            K::assert(false, "swap mmap is not a thing yet");
        }
    }
}


/**
 * This should take in the vaddr of a previously mapped page and load it into
 * memory and maps it into the user address space, and pass the kernel virtual
 * address to the continutation function as a uint64_t. This page should be
 * pinned in memory so unpin it once the kernel is done working with it so it
 * can be evicted (on not based off use case).
 */
void load_mmapped_page(UserTCB* tcb, uint64_t uvaddr, Function<void(uint64_t)> w) {
    K::assert(uvaddr % PAGE_SIZE == 0, "invalid user vaddr passed to mmap");

    tcb->supp_page_table->lock.lock([=]() {
        LocalPageLocation* local = tcb->supp_page_table->vaddr_mapping(uvaddr);
        K::assert(local != nullptr, "page being loaded has not been mmapped.");

        PageLocation* location = local->location;


        location->lock.lock([=]() {
            if (!location->present) {

                load_location(location, [=](uint64_t paddr) {
                    tcb->page_table->map_vaddr(
                        uvaddr, paddr, build_page_attributes(local),
                        [=]() { 
                                location->lock.unlock();
                                tcb->supp_page_table->lock.unlock();
                                create_event(w, paddr_to_vaddr(paddr));
                        });
                });
                return;
            } else {
                pin_frame(location->paddr);
                tcb->page_table->map_vaddr(
                    uvaddr, location->paddr, build_page_attributes(local),
                    [=]() {
                            location->lock.unlock();
                            tcb->supp_page_table->lock.unlock();
                            create_event(w, paddr_to_vaddr(location->paddr));
                    });
                return;
            }
        });
        return;
    });
    return;
}

/**
 * fills in necessary tcb datastructures with info for mmapping of a page and
 * runs the continuation function. This does not load the region into memory, which
 * can be done by load_mmapped_region.
 */
void mmap(UserTCB* tcb, uint64_t uvaddr, int prot, int flags, file* file, uint64_t offset,
          int length, Function<void(void)> w) {
    K::assert(uvaddr % PAGE_SIZE == 0, "invalid user vaddr being mmapped");
    K::assert(length > 0, "invalid length input into mmap");

    int num_mappings = length / PAGE_SIZE;
    if (length % PAGE_SIZE != 0) {
        num_mappings += 1;
    }

    Semaphore* sema = new Semaphore((-1 * num_mappings) + 1);
    int swap_offset;

    for (int i = 0; i < num_mappings; i++) {
        if ((flags & MAP_ANONYMOUS) != 0) {
            if ((flags & MAP_NORESERVE) != 0) {
                swap_offset = 1;
            } else {
                swap_offset = 0;
            }
        } else {
            swap_offset = offset + (i * PAGE_SIZE);
        }

        mmap_page(tcb, uvaddr + (i * PAGE_SIZE), prot, flags, file, swap_offset, 0,
                  [=]() { sema->up(); });
    }

    sema->down([=]() {
        delete sema;
        create_event(w);
    });
}
