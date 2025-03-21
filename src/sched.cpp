#include "sched.h"

#include "heap.h"
#include "irq.h"
#include "printf.h"
#include "queue.h"

static struct task_struct init_task = INIT_TASK;
struct task_struct* current = &(init_task);
struct task_struct* task[NR_TASKS] = {
    &(init_task),
};
int nr_tasks = 1;

void preempt_disable(void) {
    current->preempt_count++;
}

void preempt_enable(void) {
    current->preempt_count--;
}

void _schedule(void) {
    preempt_disable();
    // int next,c;
    // struct task_struct * p;
    // while (1) {
    // 	c = -1;
    // 	next = 0;
    // 	for (int i = 0; i < NR_TASKS; i++){
    // 		p = task[i];
    // 		if (p && p->state == TASK_RUNNING && p->counter > c) {
    // 			c = p->counter;
    // 			next = i;
    // 		}
    // 	}
    // 	if (c) {
    // 		break;
    // 	}
    // 	for (int i = 0; i < NR_TASKS; i++) {
    // 		p = task[i];
    // 		if (p) {
    // 			p->counter = (p->counter >> 1) + p->priority;
    // 		}
    // 	}
    // }
    // switch_to(task[next]);
    preempt_enable();
}

void schedule(void) {
    current->counter = 0;
    _schedule();
}

extern "C" void switch_to(struct task_struct* next) {
    if (current == next) return;
    struct task_struct* prev = current;
    current = next;
    cpu_switch_to(prev, next);
}

extern "C" void schedule_tail(void) {
    preempt_enable();
}

extern "C" void timer_tick() {
    --current->counter;
    if (current->counter > 0 || current->preempt_count > 0) {
        return;
    }
    current->counter = 0;
    enable_irq();
    _schedule();
    disable_irq();
}
