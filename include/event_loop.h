#ifndef _EVENT_LOOP_H
#define _EVENT_LOOP_H

#include "atomic.h"
#include "event.h"
#include "percpu.h"
#include "queue.h"
#include "atomic.h"
#include "threads.h"

#define MAX_PRIORITY 5

struct percpu_queue {
    SpinLock lock;
    queue<event*>* queue_list[MAX_PRIORITY];
};

extern PerCPU<percpu_queue> cpu_queues;

inline void create_event(Function<void()> w, int priority)
{
    event *e = new event_work(w);
    cpu_queues.mine().queue_list[priority]->push(e);
}

template <typename T>
inline void create_event_value(Function<void(T)> w, T value, int priority) {
    event* e = new event_work_value<T>(w, value);
    cpu_queues.mine().queue_list[priority]->push(e);
}

extern event* pop(int cpu);
void loop();

#endif