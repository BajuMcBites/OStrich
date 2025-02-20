#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include "event.h"
#include "percpu.h"
#include "queue.h"
#include "atomic.h"

#define MAX_PRIORITY            5

struct percpu_queue {
	SpinLock lock;
	queue<event*>* queue_list[MAX_PRIORITY];
};

extern PerCPU<percpu_queue> cpu_queues;

template <typename work>
void create_event(work w, int priority) {
    event* e = new event_work<work>(w);
    cpu_queues.mine().queue_list[priority]->push(e);
}

template <typename work, typename T>
void create_event_value(work w, T* value, int priority) {
    event* e = new event_work_value<work, T*>(w, value);
    cpu_queues.mine().queue_list[priority]->push(e);
}

extern event* pop(int cpu);
void loop();


#endif 