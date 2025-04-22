
#include "event.h"

#include "atomic.h"
#include "libk.h"
#include "percpu.h"
#include "printf.h"
#include "queue.h"
#include "stdint.h"
#include "sys.h"
#include "utils.h"

extern "C" void load_user_context(cpu_context* context);
extern "C" uint64_t pickKernelStack(void);
extern "C" uint64_t get_sp_el0();
extern "C" uint64_t get_elr_el1();
extern "C" uint64_t get_spsr_el1();

// queue of TCBs ready to run
PerCPU<CPU_Queues> readyQueue;

// use an array for multiple cores, index with getCoreId
UserTCB* runningUserTCB[CORE_COUNT] = {nullptr};

// use an array for multiple cores, index with getCoreId
TCB* runningEvent[CORE_COUNT] = {nullptr};

TCB* getNextEvent(int core) {
    auto& ready = readyQueue.forCPU(core);
    for (int i = 0; i < PRIORITY_LEVELS; i++) {
        auto next = ready.queues[i].remove();
        if (next != nullptr) {
            return next;
        }
    }
    return nullptr;
}

/**
 * This function is the main event loop. It runs in a loop, checking for events
 * to run. If there are no events, it will check the next core for events.
 * If there are still no events, it will continue to spin.
 */
void run_events() {
    TCB* nextThread;
    int me = getCoreID();

    while (true) {
        nextThread = getNextEvent(me);

        if (nextThread == nullptr) {
            int nextCore = (me + 1) % CORE_COUNT;
            while (nextCore != me && nextThread == nullptr) {
                nextThread = getNextEvent(nextCore);
                nextCore = (nextCore + 1) % CORE_COUNT;  // Move to the next core
            }

            if (nextThread == nullptr)  // no other threads. I can keep working/spinning
            {
                continue;
            }
        }
        runningEvent[getCoreID()] = nextThread;
        Interrupts::restore(nextThread->irq_was_disabled);
        nextThread->run();
        if (!nextThread->kernel_event) {
            K::assert(false, "user event returned to event loop");
        }

        delete nextThread; /* only kernel events so UserTCB's shouldn't be deleted*/
    }
}

/**
 * This function should reset the stack pointer and then jump to the event loop
 * -> can be called anywhere without being recursive since the stack pointer is reset
 */
void event_loop() {
    uint64_t sp = pickKernelStack();
    set_sp_and_jump(sp - 0x20, run_events);
}

/**
 * This function is called from the kernel to enter user space. It loads the
 * before loading the user context of the tcb and eret-ing
 */
void enter_user_space(UserTCB* tcb) {
    tcb->pcb->page_table->use_page_table();
    runningUserTCB[getCoreID()] = tcb;
    runningEvent[getCoreID()] = tcb;
    load_user_context(&tcb->context);
}

/**
 * This function is called from the user space to save the context of the user
 * thread. It saves the registers and the stack pointer, program counter, and
 * spsr, these registers should be saved immidiatly upon entering kernel space
 * so call this function at the start of the saved register memory.
 */
void save_user_context(UserTCB* tcb, KernelEntryFrame* regs) {
    tcb->context.x0 = regs->X[0];
    tcb->context.x1 = regs->X[1];
    tcb->context.x2 = regs->X[2];
    tcb->context.x3 = regs->X[3];
    tcb->context.x4 = regs->X[4];
    tcb->context.x5 = regs->X[5];
    tcb->context.x6 = regs->X[6];
    tcb->context.x7 = regs->X[7];
    tcb->context.x8 = regs->X[8];
    tcb->context.x9 = regs->X[9];
    tcb->context.x10 = regs->X[10];
    tcb->context.x11 = regs->X[11];
    tcb->context.x12 = regs->X[12];
    tcb->context.x13 = regs->X[13];
    tcb->context.x14 = regs->X[14];
    tcb->context.x15 = regs->X[15];
    tcb->context.x16 = regs->X[16];
    tcb->context.x17 = regs->X[17];
    tcb->context.x18 = regs->X[18];
    tcb->context.x19 = regs->X[19];
    tcb->context.x20 = regs->X[20];
    tcb->context.x21 = regs->X[21];
    tcb->context.x22 = regs->X[22];
    tcb->context.x23 = regs->X[23];
    tcb->context.x24 = regs->X[24];
    tcb->context.x25 = regs->X[25];
    tcb->context.x26 = regs->X[26];
    tcb->context.x27 = regs->X[27];
    tcb->context.x28 = regs->X[28];
    tcb->context.x29 = regs->X[29];
    tcb->context.x30 = regs->X[30];

    tcb->context.sp = get_sp_el0();
    tcb->context.pc = get_elr_el1();
    tcb->context.spsr = get_spsr_el1();
}

void yield(KernelEntryFrame* frame) {
    K::assert(Interrupts::isDisabled(), "preempt happening with interrupts on uh oh\n");
    TCB* running = runningEvent[getCoreID()];
    if (running->kernel_event) {
        // check stack
        QA7->TimerClearReload.IntClear = 1;  // Clear interrupt
        QA7->TimerClearReload.Reload = 1;    // Reload now
        Interrupts::restore(running->irq_was_disabled);
        return;
    }

    UserTCB* old = get_running_user_tcb(getCoreID());
    // force thread to change
    K::assert((running == old), "mismatched running event and user thread\n");

    save_user_context(old, frame);
    QA7->TimerClearReload.IntClear = 1;  // Clear interrupt
    QA7->TimerClearReload.Reload = 1;    // Reload now
    enable_irq();
    event_loop();
}

void set_return_value(UserTCB* tcb, uint64_t ret_val) {
    tcb->context.x0 = ret_val;
}

UserTCB* get_running_user_tcb(int core) {
    return runningUserTCB[core];
}

TCB* get_running_task(int core) {
    return runningEvent[core];
}
