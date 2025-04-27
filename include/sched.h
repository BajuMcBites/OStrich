#ifndef _SCHED_H
#define _SCHED_H

#define THREAD_CPU_CONTEXT 0  // offset of cpu_context in task_struct

#ifndef __ASSEMBLER__

#define THREAD_SIZE 4096

#define TASK_RUNNING 0

#include "event.h"

extern "C" void user_entry(cpu_context* context);
extern "C" void save_user_state(cpu_context* context);
extern "C" void load_user_context(cpu_context* context);
extern "C" uint64_t get_sp_el0();
extern "C" uint64_t get_elr_el1();
extern "C" uint64_t get_spsr_el1();

#endif
#endif