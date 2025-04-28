#include "event.h"
#include "frame.h"
#include "mmap.h"
#include "printf.h"
#include "swap.h"
#include "sys.h"
#include "trap_frame.h"

extern "C" uint64_t get_sp_el0();
extern "C" uint64_t get_elr_el1();
extern "C" uint64_t get_spsr_el1();
extern "C" uint64_t get_far_el1();
extern "C" uint64_t get_esr_el1();

extern PageCache* page_cache;
extern Swap* swap;

void handle_translation_fault(KernelEntryFrame* trap_frame, uint64_t esr, uint64_t elr,
                              uint64_t spsr, uint64_t far);
void handle_permissions_fault(KernelEntryFrame* trap_frame, uint64_t esr, uint64_t elr,
                              uint64_t spsr, uint64_t far);

extern "C" void page_fault_handler(KernelEntryFrame* trap_frame, uint64_t esr, uint64_t elr,
                                   uint64_t spsr, uint64_t far) {
    UserTCB* tcb = get_running_user_tcb(getCoreID());
    save_user_context(tcb, trap_frame);

    switch ((esr >> 2) & 0x3) {
        case 0:
            printf_err("page_fault: Address size fault\n");
            printf_err(":\n  ESR_EL1 0x%X%X ELR_EL1 0x%X%X\n SPSR_EL1 0%X%X FAR_EL1 0x%X%X\n",
                       esr >> 32, esr, elr >> 32, elr, spsr >> 32, spsr, far >> 32, far);
            K::assert(false, "Not handled yet\n");
            break;
        case 1:
            // printf_err("Translation fault: %x\n", esr);
            handle_translation_fault(trap_frame, esr, elr, spsr, far);
            break;
        case 3:
            // printf_err("Permission fault: %x\n", esr);
            handle_permissions_fault(trap_frame, esr, elr, spsr, far);
            break;
        default:
            printf_err("Unkown fault");
            K::assert(false, "Not handled yet\n");
    }

    switch (esr >> 26) {
        case 0b100000:
            printf_err("Instruction abort, lower EL\n");
            handle_translation_fault(trap_frame, esr, elr, spsr, far);
            break;
        default:
            K::assert(false, "Fault not handled Error\n");
            break;
    }

    K::assert(false, "Shouldnt Get Here\n");
}

void handle_translation_fault(KernelEntryFrame* trap_frame, uint64_t esr, uint64_t elr,
                              uint64_t spsr, uint64_t far) {
    uint64_t user_sp = get_sp_el0();

    UserTCB* tcb = get_running_user_tcb(getCoreID());
    save_user_context(tcb, trap_frame);

    if (far == nullptr) {
        kill_process(tcb->pcb);
        event_loop();
    }

    /* try loading in the page*/
    load_mmapped_page(tcb->pcb, far & (~0xFFF), [=](uint64_t kvaddr) {
        if (kvaddr == 0) { /* page isnt mapped */
            /* map it as a new swap page (stack growth) and load it in */
            mmap_page(tcb->pcb, far & (~0xFFF), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE,
                      nullptr, 0, 0, [=]() {
                          load_mmapped_page(tcb->pcb, far & (~0xFFF), [=](uint64_t kvaddr) {
                              unpin_frame(vaddr_to_paddr(kvaddr));
                              queue_user_tcb(tcb);
                          });
                      });
        } else { /* page is mapped and now loaded into memory, unpin and requeu*/
            unpin_frame(kvaddr);
            queue_user_tcb(tcb);
        }
    });

    event_loop();
}

void handle_permissions_fault(KernelEntryFrame* trap_frame, uint64_t esr, uint64_t elr,
                              uint64_t spsr, uint64_t far) {
    uint64_t user_sp = get_sp_el0();

    UserTCB* tcb = get_running_user_tcb(getCoreID());
    PCB* pcb = tcb->pcb;

    save_user_context(tcb, trap_frame);

    pcb->supp_page_table->lock.lock([=]() {
        LocalPageLocation* local = pcb->supp_page_table->vaddr_mapping(far & ~0xFFF);
        PageLocation* location = local->location;

        /* check if we actually have write permisisons on the page */
        if ((local->perm & WRITE_PERM == 0 && (esr >> 6) & 0x1) || (local->perm & READ_PERM == 0)) {
            pcb->supp_page_table->lock.unlock();
            kill_process(pcb);
            event_loop();
        }

        pcb->supp_page_table->lock.unlock();
        /* load the page we faulted on into memory */
        load_mmapped_page(pcb, far & ~0xFFF, [=](uint64_t kvaddr_old) {
            location->lock.lock([=]() {
                /* if we are the only user and NOT a private filesys page we can just use it */
                if (location->ref_count == 1 && location->location_type != FILESYSTEM) {
                    unpin_frame(vaddr_to_paddr(kvaddr_old));
                    queue_user_tcb(tcb);
                    location->lock.unlock();
                    return;
                } else { /* copy on write */
                    /* create a new local mapping to replace with */
                    LocalPageLocation* new_local =
                        new LocalPageLocation(pcb, local->perm, local->sharing_mode, local->uvaddr);

                    swap->get_swap_id([=](uint64_t new_id) {
                        page_cache->get_or_add(
                            nullptr, 0, new_id, new_local, [=](PageLocation* new_location) {
                                /* replace mapping with new local */
                                pcb->supp_page_table->map_vaddr(new_local->uvaddr, new_local);
                                load_mmapped_page(pcb, far & ~0xFFF, [=](uint64_t kvaddr_new) {
                                    /* copy the old page to the new page */
                                    memcpy((void*)kvaddr_new, (void*)kvaddr_old, PAGE_SIZE);

                                    /* unpin both pages */
                                    unpin_frame(vaddr_to_paddr(kvaddr_new));
                                    unpin_frame(vaddr_to_paddr(kvaddr_old));

                                    /* delete the old page mapping impliciptly releases lock*/
                                    page_cache->remove(local, [=]() { delete local; });

                                    queue_user_tcb(tcb);
                                });
                            });
                    });
                }
            });
        });
    });
    event_loop();
}