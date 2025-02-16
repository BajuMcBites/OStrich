#ifndef _SCHED_H
#define _SCHED_H

#define THREAD_CPU_CONTEXT			0 		// offset of cpu_context in task_struct 

#ifndef __ASSEMBLER__

#define THREAD_SIZE				4096

#define NR_TASKS				64 

#define TASK_RUNNING				0

#define MAX_PRIORITY            5

#include "atomic.h" // this gotta be bad practice surely
#include "percpu.h" // this too...


extern struct task_struct *current;
extern struct task_struct * task[NR_TASKS];
extern int nr_tasks;

struct cpu_context {
	unsigned long x19;
	unsigned long x20;
	unsigned long x21;
	unsigned long x22;
	unsigned long x23;
	unsigned long x24;
	unsigned long x25;
	unsigned long x26;
	unsigned long x27;
	unsigned long x28;
	unsigned long fp;
	unsigned long sp;
	unsigned long pc;
};

struct task_struct {
	struct cpu_context cpu_context;
	long state;	
	long counter;
	long priority;
	long preempt_count;
};


// structs used for closures for event-driven scheduler

struct event_struct {
	void (*func)(void*);
	void* arg;
	long priority;
	struct event_struct* next;
};

struct percpu_queue {
	SpinLock lock;
	struct event_struct* queue_list[MAX_PRIORITY];
};

extern PerCPU<percpu_queue> cpu_queues;

extern void sched_init(void);
extern void schedule(void);
extern void push(int cpu, event_struct* e);
extern struct event_struct* pop(int cpu);
extern "C" void timer_tick(void);
extern "C" void preempt_disable(void);
extern "C" void preempt_enable(void);
extern "C" void switch_to(struct task_struct* next);
extern "C" void cpu_switch_to(struct task_struct* prev, struct task_struct* next);

#define INIT_TASK \
{ {0,0,0,0,0,0,0,0,0,0,0,0,0}, \
0,0,1,0 \
}

#endif
#endif