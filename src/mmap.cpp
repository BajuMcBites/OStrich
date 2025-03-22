#include "mmap.h"
#include "frame.h"
#include "function.h"

// /**
//  *  This assumes that the tcb->sup->local_loc->loc page is not resident in memory,
//  *  passes the kernel virtual address where the page was loaded into w.
//  *  **NOTE 
//  *         the tcb->supp talbe, tcb's supp_page_table[vaddr]->localPageLocaiton.location_lock and
//  *         localPageLocaiton->location.lock should both be held to call this 
//  *         function
//  */
// void load_mmapped_page(uint64_t uvaddr, UserTCB* tcb, Function<void(void*)> w) {

//     LocalPageLocation* local = tcb->supp_page_table->vaddr_mapping(uvaddr);

//     K::assert(local != nullptr, "vaddr not mapped");

//     PageLocation* location = local->location;

//     K::assert(!location->present, "we are trying to load an already loaded page");

//     alloc_frame(PINNED_PAGE_FLAG, location, [=](uint64_t paddr) {

//         void* page_vaddr = (void*) paddr;
//         K::memset(page_vaddr, 0, PAGE_SIZE); //dont give none zero memory
        
//         tcb->page_table->map_vaddr(uvaddr, paddr, 0x1, [=]() { /* TODO figure out lower attributes from locals flags */

//             if (location->location_type == ANONYMOUS) { /* not backed page */

//                 location->present = true;
//                 location->paddr = paddr;
//                 create_event(w, page_vaddr);
//                 return;
    
//             } else if (location->location_type == SWAP) { /* backed page*/

//                 K::assert(false, "swap location type not implemented");

//             } else if (location->location_type == FILESYSTEM) {

//                 FileLocation* file_location = location->location.filesystem;
//                 read(file_location->file_name, file_location->offset, (char*) page_vaddr, PAGE_SIZE, [=](int ret) {
//                     K::assert(ret >= 0, "read failed");
//                     location->paddr = paddr;
//                     location->present = true;

//                     create_event(w, page_vaddr);
//                 });

//             } else {
//                 K::assert(false, "invalid location type");
//             }
//         });
//         return;
//     });
//     return;
// }

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

        void* page_vaddr = (void*) paddr;
        K::memset(page_vaddr, 0, PAGE_SIZE); //dont give none zero memory

        if (location->location_type == ANONYMOUS) { /* not backed page */

            location->present = true;
            location->paddr = paddr;
            create_event(w, paddr);
            return;

        } else if (location->location_type == SWAP) { /* backed page*/

            K::assert(false, "swap location type not implemented");

        } else if (location->location_type == FILESYSTEM) {

            FileLocation* file_location = location->location.filesystem;
            read(file_location->file_name, file_location->offset, (char*) page_vaddr, PAGE_SIZE, [=](int ret) {
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
void mmap(UserTCB* tcb, uint64_t uvaddr, int prot, MMAPFlag flags, char* file_name, uint64_t offset, Function<void(void)> w) {

    K::assert(uvaddr % PAGE_SIZE == 0, "invalid user vaddr passed to mmap");

    tcb->supp_page_table->lock.lock([=]() {

        LocalPageLocation* local = tcb->supp_page_table->vaddr_mapping(uvaddr);
        K::assert(local == nullptr, "remapping an already mapped address");

        local = new LocalPageLocation;
        /* TODO: set locals flags appropriately, rn setting with the below using prot */
        local->read_only = false;
        local->sharing_mode = PRIVATE;
        local->next = nullptr;

        PageLocation* location = nullptr;
        
        /* TODO: insert page cache look for page (atomic get or create function), return with refcount -1 if newly created */

        location = new PageLocation;
        location->ref_count = -1; /* simulating getting new one from page cache*/

        location->lock.lock([=]() {
            if (location->ref_count == -1) {
                local->location = location;
                location->ref_count = 1;
                location->users = local;
                location->present = false;

                if (flags == FILESYSTEM_m) {
                    
                    K::assert(file_name != nullptr, "mmapping nullptr file");
                    
                    location->location_type = FILESYSTEM;
                    location->location.filesystem = new FileLocation(file_name, offset);
                    tcb->supp_page_table->map_vaddr(uvaddr, local);

                    location->lock.unlock();
                    tcb->supp_page_table->lock.unlock();
                    create_event(w);
                    return;
                } else if (flags == SWAP_m) {

                    K::assert(false, "swap mmap not implemented yet");
                    return;
                } else if (flags == ANONYMOUS_m) {
                    location->location_type = ANONYMOUS;
                    tcb->supp_page_table->map_vaddr(uvaddr, local);

                    location->lock.unlock();
                    tcb->supp_page_table->lock.unlock();
                    create_event(w);
                    return;
                } else {
                    K::assert(false, "invalid mmap flag passed");
                    return;
                }
                K::assert(false, "mmap: we should not get here");
            } else {
                /* TODO add local to locations stuff and increase ref count + alter linked list*/

                return;
            }
        });
        return;

    });
    return;
}

/**
 * This should take in the vaddr of a previously mapped page and load it into 
 * memory, and pass the kernel virtual address to the continutation function as
 * a uint64_t. This page should be pinned in memory so unpin it once the kernel is
 * done working with it so it can be evicted (on not based off use case).
 */
void load_mmapped_page(UserTCB* tcb, uint64_t uvaddr, Function<void(uint64_t)> w) {

    K::assert(uvaddr % PAGE_SIZE == 0, "invalid user vaddr passed to mmap");

    tcb->supp_page_table->lock.lock([=]() {
        LocalPageLocation* local = tcb->supp_page_table->vaddr_mapping(uvaddr);
        K::assert(local != nullptr, "page being loaded has not been mmapped.");

        local->location_lock.lock([=]() {

            PageLocation* location = local->location;

            location->lock.lock([=]() {

                if (!location->present) {
                    load_location(location, [=](uint64_t paddr) {
                        tcb->page_table->map_vaddr(uvaddr, paddr, 0x1, [=]() { /* todo figure local page attributes out from local page*/
                            location->lock.unlock();
                            local->location_lock.unlock();
                            tcb->supp_page_table->lock.unlock();
                            create_event(w, paddr_to_vaddr(paddr));
                        });
                    });
                    return;
                } else {
                    pin_frame(location->paddr);
                    tcb->page_table->map_vaddr(uvaddr, location->paddr, 0x1, [=]() { /* todo figure local page attributes out from local page*/
                        location->lock.unlock();
                        local->location_lock.unlock();
                        tcb->supp_page_table->lock.unlock();
                        create_event(w, paddr_to_vaddr(location->paddr));
                    });
                    return;
                }
            });
            return;
        });
        return;
    });
    return;
}


