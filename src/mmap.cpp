#include "mmap.h"

#include "frame.h"
#include "function.h"

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
            read(file_location->file_name, file_location->offset, (char*)page_vaddr, PAGE_SIZE,
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

/**
 * fills in necessary tcb datastructures with info for mmapping a SINGLE page and
 * runs the continuation function. This does not load the page into memory, which
 * can be done by load_mmapped_page.
 */
void mmap_page(UserTCB* tcb, uint64_t uvaddr, int prot, int flags, char* file_name, uint64_t offset,
               Function<void(void)> w) {
    bool file_mmap = (flags & MAP_ANONYMOUS) == 0;
    bool no_reserve = (flags & MAP_NORESERVE) != 0;

    K::assert(uvaddr % PAGE_SIZE == 0, "invalid user vaddr passed to mmap");

    tcb->supp_page_table->lock.lock([=]() {
        LocalPageLocation* local = tcb->supp_page_table->vaddr_mapping(uvaddr);
        // if (local != nullptr) return;
        K::assert(local == nullptr, "remapping an already mapped address");

        local = new LocalPageLocation;
        local->perm = prot;
        local->next = nullptr;

        if ((flags & 0x3) == MAP_SHARED) {
            local->sharing_mode = SHARED;
        } else if ((flags & 0x3) == MAP_PRIVATE) {
            local->sharing_mode = PRIVATE;
        }

        PageLocation* location = nullptr;

        /* TODO: insert page cache look for page (atomic get or create function), return with
         * refcount -1 if newly created */

        location = new PageLocation;
        location->ref_count = -1; /* simulating getting new one from page cache*/

        location->lock.lock([=]() {
            if (location->ref_count == -1) {
                local->location = location;
                location->ref_count = 1;
                location->users = local;
                location->present = false;

                if (file_mmap) {
                    K::assert(file_name != nullptr, "mmapping nullptr file");

                    location->location_type = FILESYSTEM;
                    location->location.filesystem = new FileLocation(file_name, offset);
                    tcb->supp_page_table->map_vaddr(uvaddr, local);
                    location->lock.unlock();
                    tcb->supp_page_table->lock.unlock();
                    create_event(w);
                    return;
                } else if (!file_mmap) {
                    if (!no_reserve) {
                        K::assert(false, "swap mmap not implemented yet");

                    } else {
                        location->location_type = UNBACKED;
                        tcb->supp_page_table->map_vaddr(uvaddr, local);
                        location->lock.unlock();
                        tcb->supp_page_table->lock.unlock();
                        create_event(w);
                        return;
                    }
                    return;
                }
                K::assert(false, "mmap: we should not get here");
            } else {
                /* TODO add local to locations stuff and increase ref count + alter linked list*/
                K::assert(false, "no implemnetation for two pages mapped to same location");
                return;
            }
        });
        return;
    });
    return;
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

        // local->location_lock.lock([=]() { /* supp table lock covers this race condition it think
        // */

        PageLocation* location = local->location;

        location->lock.lock([=]() {
            if (!location->present) {
                load_location(location, [=](uint64_t paddr) {
                    tcb->page_table->map_vaddr(
                        uvaddr, paddr, build_page_attributes(local),
                        [=]() { /* todo figure local page attributes out from local page*/
                                location->lock.unlock();
                                // local->location_lock.unlock();
                                tcb->supp_page_table->lock.unlock();
                                create_event(w, paddr_to_vaddr(paddr));
                        });
                });
                return;
            } else {
                pin_frame(location->paddr);
                tcb->page_table->map_vaddr(
                    uvaddr, location->paddr, build_page_attributes(local),
                    [=]() { /* todo figure local page attributes out from local page*/
                            location->lock.unlock();
                            // local->location_lock.unlock();
                            tcb->supp_page_table->lock.unlock();
                            create_event(w, paddr_to_vaddr(location->paddr));
                    });
                return;
            }
        });
        return;
        // });
        // return;
    });
    return;
}

/**
 * fills in necessary tcb datastructures with info for mmapping of a page and
 * runs the continuation function. This does not load the region into memory, which
 * can be done by load_mmapped_region.
 */
void mmap(UserTCB* tcb, uint64_t uvaddr, int prot, int flags, char* file_name, uint64_t offset,
          int length, Function<void(void)> w) {
    K::assert(uvaddr % PAGE_SIZE == 0, "invalid user vaddr being mmapped");
    K::assert(length > 0, "invalid length input into mmap");

    int num_mappings = length / PAGE_SIZE;
    if (length % PAGE_SIZE != 0) {
        num_mappings += 1;
    }

    Semaphore* sema = new Semaphore((-1 * num_mappings) + 1);

    for (int i = 0; i < num_mappings; i++) {
        mmap_page(tcb, uvaddr + (i * PAGE_SIZE), prot, flags, file_name, offset + (i * PAGE_SIZE),
                  [=]() { sema->up(); });
    }

    sema->down([=]() {
        delete sema;
        create_event(w);
    });
}
