#include "event.h"
#include "frame.h"
#include "mmap.h"
#include "printf.h"
#include "swap.h"
#include "trap_frame.h"

extern "C" uint64_t get_sp_el0();
extern "C" uint64_t get_elr_el1();
extern "C" uint64_t get_spsr_el1();
extern "C" uint64_t get_far_el1();
extern "C" uint64_t get_esr_el1();

extern PageCache* page_cache;
extern Swap* swap;

void handle_translation_fault(trap_frame* trap_frame, uint64_t esr, uint64_t elr, uint64_t spsr, uint64_t far);
void handle_permissions_fault(trap_frame* trap_frame, uint64_t esr, uint64_t elr, uint64_t spsr, uint64_t far);

extern "C" void page_fault_handler(trap_frame* trap_frame, uint64_t esr, uint64_t elr, uint64_t spsr, uint64_t far) {
    UserTCB* tcb = get_running_user_tcb(getCoreID());
    save_user_context(tcb, &trap_frame->X[0]);

    switch ((esr >> 2) & 0x3) {
        case 0:
            printf_err("Address size fault\n");
            printf(":\n  ESR_EL1 0x%X%X ELR_EL1 0x%X%X\n SPSR_EL1 0%X%X FAR_EL1 0x%X%X\n", esr >> 32, esr,
                   elr >> 32, elr, spsr >> 32, spsr, far >> 32, far);
            K::assert(false, "Not handled yet\n");
            break;
        case 1:
            printf_err("Translation fault\n");
            handle_translation_fault(trap_frame, esr, elr, spsr, far);
            break;
        case 3:
            printf_err("Permission fault\n");
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

    K::assert(false, "NOOOOOOO\n");
}

void handle_translation_fault(trap_frame* trap_frame, uint64_t esr, uint64_t elr, uint64_t spsr, uint64_t far) {
    uint64_t user_sp = get_sp_el0();

    UserTCB* tcb = get_running_user_tcb(getCoreID());
    save_user_context(tcb, &trap_frame->X[0]);

    if (far == nullptr) {
        K::assert(false, "Derefencing a null, kill user process\n");
    }

    load_mmapped_page(tcb->pcb, far & (~0xFFF), [=](uint64_t kvaddr) {
        printf("%x%x this is kvaddr\n", kvaddr >> 32, kvaddr);
        if (kvaddr == 0) {
            mmap_page(tcb->pcb, far & (~0xFFF), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE,
                      nullptr, 0, 0, [=]() {
                          load_mmapped_page(tcb->pcb, far & (~0xFFF), [=](uint64_t kvaddr) {
                              unpin_frame(vaddr_to_paddr(kvaddr));
                              queue_user_tcb(tcb);
                          });
                      });
        } else {
            unpin_frame(kvaddr);
            queue_user_tcb(tcb);
        }
    });

    event_loop();
}

void handle_permissions_fault(trap_frame* trap_frame, uint64_t esr, uint64_t elr, uint64_t spsr, uint64_t far) {
    uint64_t user_sp = get_sp_el0();

    UserTCB* tcb = get_running_user_tcb(getCoreID());
    PCB* pcb = tcb->pcb;

    save_user_context(tcb, trap_frame->X);

    pcb->supp_page_table->lock.lock([=]() {
        LocalPageLocation* local = pcb->supp_page_table->vaddr_mapping(far & ~0xFFF);
        PageLocation* location = local->location;

        if ((local->perm & WRITE_PERM == 0 && (esr >> 6) & 0x1) || (local->perm & READ_PERM == 0)) {
            K::assert(false, "Need to kill process for permission fault, not implemented\n");
        }

        pcb->supp_page_table->lock.unlock();
        load_mmapped_page(pcb, far & ~0xFFF, [=](uint64_t kvaddr_old) {
            location->lock.lock([=]() {
                printf("this is our supp page table %X\n", pcb->supp_page_table);

                if (location->ref_count == 1) {
                    printf("we are the only user rn %d, refcound %d\n", pcb->pid,
                           location->ref_count);
                    unpin_frame(vaddr_to_paddr(kvaddr_old));
                    queue_user_tcb(tcb);
                    location->lock.unlock();
                    pcb->supp_page_table->lock.unlock();
                    return;
                } else {
                    printf("copy on writing %d\n", pcb->pid);

                    swap->get_swap_id([=](uint64_t new_id) {
                        page_cache->remove(local, [=]() {
                            page_cache->get_or_add(
                                nullptr, 0, new_id, local, [=](PageLocation* new_location) {
                                    load_mmapped_page(pcb, far & ~0xFFF, [=](uint64_t kvaddr_new) {
                                        memcpy((void*)kvaddr_new, (void*)kvaddr_old, PAGE_SIZE);
                                        unpin_frame(vaddr_to_paddr(kvaddr_new));
                                        unpin_frame(vaddr_to_paddr(kvaddr_old));
                                        location->lock.unlock();
                                        pcb->supp_page_table->lock.unlock();
                                        queue_user_tcb(tcb);
                                    });
                                });
                        });
                    });
                }
            });
        });
    });
    event_loop();
}