#ifndef _SCHED_H
#define _SCHED_H

#define THREAD_CPU_CONTEXT 0  // offset of cpu_context in task_struct

#ifndef __ASSEMBLER__

#define THREAD_SIZE 4096

#define NR_TASKS 64

#define TASK_RUNNING 0

#include "event.h"

extern struct task_struct* current;
extern struct task_struct* task[NR_TASKS];
extern int nr_tasks;

struct task_struct {
    struct cpu_context cpu_context;
    long state;
    long counter;
    long priority;
    long preempt_count;
};

extern void sched_init(void);
extern void schedule(void);
extern "C" void timer_tick(void);
extern "C" void preempt_disable(void);
extern "C" void preempt_enable(void);
extern "C" void switch_to(struct task_struct* next);
extern "C" void cpu_switch_to(struct task_struct* prev, struct task_struct* next);
extern "C" void user_entry(cpu_context* context);
extern "C" void save_user_state(cpu_context* context);
extern "C" void load_user_context(cpu_context* context);
extern "C" uint64_t get_sp_el0();
extern "C" uint64_t get_elr_el1();
extern "C" uint64_t get_spsr_el1();

#define INIT_TASK {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, 1, 0}

#endif
#endif