#include "process.h"

#include "printf.h"
#include "tty.h"

void fork(struct UserTCB* tcb) {
    PCB* child_pcb = new PCB();
    child_pcb->frameBuffer = request_tty();
    tcb->pcb->add_child(child_pcb);
    child_pcb->supp_page_table->copy_mappings(tcb->pcb->supp_page_table, child_pcb, [=]() {
        UserTCB* child_tcb = new UserTCB();
        child_tcb->pcb = child_pcb;
        child_tcb->frameBuffer = child_pcb->frameBuffer;
        memcpy(&child_tcb->context, &tcb->context, sizeof(cpu_context));
        child_tcb->context.x0 = 0;
        tcb->context.x0 = child_pcb->pid;
        child_pcb->page_table->alloc_pgd([=]() {
            child_tcb->state = TASK_RUNNING;
            queue_user_tcb(tcb);
            queue_user_tcb(child_tcb);
        });
    });
}

void kill_process(struct PCB* pcb) {
    Signal* s = new Signal(SIGKILL, -1, -1);
    pcb->sigs->add(s);
}