// #include "event.h"
// #include "vm.h"
// #include "utils.h"
// #include "frame.h"
// #include "fs.h"
// #include "libk.h"

// #define ADDRESS_FAULT 0
// #define TRANSLATION_FAULT 1
// #define ACCESS_FLAG_FAULT 2
// #define PERMISSION_FAULT 3

#include "event.h"
#include "printf.h"
#include "trap_frame.h"

// void handle_page_fault(int fault_el, int type, int table_level,  unsigned long far, UserTCB* tcb)
// {

//     if (fault_el != 0) {
//         return;
//     }

//     // UserTCB* tcb = (UserTCB*) currentTCB(getCoreID());

//     if (type == ADDRESS_FAULT) {

//     } else if (type == TRANSLATION_FAULT) {

//         handle_translation_fault(fault_el, table_level, far, tcb);
//         return;

//     } else if (type == ACCESS_FLAG_FAULT) {

//     } else if (type == PERMISSION_FAULT) {

//     } else {
//         /* undefined fault */
//     }

// }

// void handle_translation_fault(int fault_el, int table_level,  unsigned long far, UserTCB* tcb) {
//     tcb->supp_page_table.vaddr_mapping(far, [=](LocalPageLocation* local_page_location) {
//         if (local_page_location == nullptr) { /* unmapped memory - potential stack growth */
//             /* check stack growth*/
//         } else { /* we already have a location for this page (resident, in swap, in filesystem)
//         */
//             local_page_location->lock.lock([=]() {
//                 PageLocation* physical_location = local_page_location->location; /* actual
//                 physical location */ local_page_location->lock.unlock();
//                 physical_location->lock.lock([=]() { /* lock ensures page cannot be evicted while
//                 we are doing anything with it */
//                     if (physical_location->present) { /* in memory, we dont need to load it in*/
//                         tcb->page_table.map_vaddr(far, physical_location->paddr, 0x1, [=]() { /*
//                         map it in to user's page table - TODO: insert lower attributes*/
//                             /* TODO: requeue user tcb */
//                             physical_location->lock.unlock();
//                         });
//                         return;
//                     } else { /* need to load it in from swap of fs */
//                         physical_location->lock.unlock();
//                     }
//                 });
//                 return;
//             });
//             return;
//         }
//     });
//     return;
// }

// void load_frame_from_location(PageLocation* physical_location, unsigned long far, UserTCB* tcb) {

//     if (physical_location->location_type == FILESYSTEM) {

//     } else if (physical_location->location_type == SWAP) {

//     }
// }

// void load_frame_swap(PageLocation* physical_location, unsigned long far, UserTCB* tcb) {
//     /* TODO */
// }

// void load_frame_fs(PageLocation* physical_location, unsigned long far, UserTCB* tcb) {
//     alloc_frame(PINNED_PAGE_FLAG, [=](uint64_t paddr) {
//         uint64_t vaddr = paddr_to_vaddr(paddr);
//         int path_length = K::strnlen(physical_location->location.filesystem.file_name, PATH_MAX -
//         1); char* file_name_cpy = (char*) kmalloc(path_length + 1); K::strncpy(file_name_cpy,
//         physical_location->location.filesystem.file_name, PATH_MAX); read(file_name_cpy,
//         physical_location->location.filesystem.offset, (char* ) vaddr, PAGE_SIZE, [=]() {
//             kfree(file_name_cpy);
//             unpin_frame(paddr);

//             physical_location->lock.unlock();
//         });

//     });
//     return;
// }

extern "C" void page_fault_handler(trap_frame* trap_frame) {
    UserTCB* tcb = get_running_user_tcb(getCoreID());
    save_user_context(tcb, &trap_frame->X[0]);

    printf("in data abort handler\n");
}