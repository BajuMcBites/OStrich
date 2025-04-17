
#include "event.h"

#include "atomic.h"
#include "libk.h"
#include "percpu.h"
#include "printf.h"
#include "queue.h"
#include "stdint.h"
#include "utils.h"

extern "C" void load_user_context(cpu_context* context);
extern "C" uint64_t pickKernelStack(void);
extern "C" uint64_t get_sp_el0();
extern "C" uint64_t get_elr_el1();
extern "C" uint64_t get_spsr_el1();


struct PCB* task[NR_TASKS] = {nullptr};
int curr_task = 0;

// queue of TCBs ready to run
PerCPU<CPU_Queues> readyQueue;

// use an array for multiple cores, index with getCoreId
UserTCB* runningUserTCB[CORE_COUNT] = {nullptr};

TCB* getNextEvent(int core) {
    auto& ready = readyQueue.forCPU(core);
    for (int i = 0; i < PRIORITY_LEVELS; i++) {
        while (1) {
        auto next = ready.queues[i].remove();
            if (next != nullptr) {
                if (next->state == TASK_RUNNING) 
                    return next;
                else if (next->state == TASK_STOPPED) {
                    ready.queues[i].add(next);
                } else if (next->state == TASK_KILLED) {
                    delete next;
                    continue;
                }
            } else break;
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
            // int nextCore = (me + 1) % CORE_COUNT;
            // while (nextCore != me && nextThread == nullptr) {
            //     nextThread = getNextEvent(nextCore);
            //     nextCore = (nextCore + 1) % CORE_COUNT;  // Move to the next core
            // }

            // if (nextThread == nullptr)  // no other threads. I can keep working/spinning
            // {
            //     continue;
            // }
            continue;
        }

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
    printf("we went to user, %d this is pid, %x this is pc\n", tcb->pcb->pid, tcb->context.pc);
    tcb->pcb->page_table->use_page_table();
    runningUserTCB[getCoreID()] = tcb;
    load_user_context(&tcb->context);
}

/**
 * This function is called from the user space to save the context of the user
 * thread. It saves the registers and the stack pointer, program counter, and
 * spsr, these registers should be saved immidiatly upon entering kernel space
 * so call this function at the start of the saved register memory.
 */
void save_user_context(UserTCB* tcb, uint64_t* regs) {
    tcb->context.x0 = regs[0];
    tcb->context.x1 = regs[1];
    tcb->context.x2 = regs[2];
    tcb->context.x3 = regs[3];
    tcb->context.x4 = regs[4];
    tcb->context.x5 = regs[5];
    tcb->context.x6 = regs[6];
    tcb->context.x7 = regs[7];
    tcb->context.x8 = regs[8];
    tcb->context.x9 = regs[9];
    tcb->context.x10 = regs[10];
    tcb->context.x11 = regs[11];
    tcb->context.x12 = regs[12];
    tcb->context.x13 = regs[13];
    tcb->context.x14 = regs[14];
    tcb->context.x15 = regs[15];
    tcb->context.x16 = regs[16];
    tcb->context.x17 = regs[17];
    tcb->context.x18 = regs[18];
    tcb->context.x19 = regs[19];
    tcb->context.x20 = regs[20];
    tcb->context.x21 = regs[21];
    tcb->context.x22 = regs[22];
    tcb->context.x23 = regs[23];
    tcb->context.x24 = regs[24];
    tcb->context.x25 = regs[25];
    tcb->context.x26 = regs[26];
    tcb->context.x27 = regs[27];
    tcb->context.x28 = regs[28];
    tcb->context.x29 = regs[29];
    tcb->context.x30 = regs[30];

    tcb->context.sp = get_sp_el0();
    tcb->context.pc = get_elr_el1();
    tcb->context.spsr = get_spsr_el1();
}

void set_return_value(UserTCB* tcb, uint64_t ret_val) {
    tcb->context.x0 = ret_val;
}

UserTCB* get_running_user_tcb(int core) {
    return runningUserTCB[core];
}
