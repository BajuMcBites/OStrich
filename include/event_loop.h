#ifndef _EVENT_LOOP_H
#define _EVENT_LOOP_H

#include "atomic.h"
#include "event.h"
#include "percpu.h"
#include "queue.h"
#include "function.h"
#include "printf.h"

#define MAX_PRIORITY 5

struct percpu_queue {
    SpinLock lock;
    queue<event*>* queue_list[MAX_PRIORITY];
};

extern PerCPU<percpu_queue> cpu_queues;

inline void create_event(Function<void()> w, int priority) {
    event* e = new event_work(w);
    cpu_queues.mine().lock.lock();
    cpu_queues.mine().queue_list[priority]->push(e);
    cpu_queues.mine().lock.unlock();
}

template <typename T>
inline void create_event_value(Function<void(T)> w, T value, int priority) {
    event* e = new event_work_value<T>(w, value);
    cpu_queues.mine().lock.lock();
    cpu_queues.mine().queue_list[priority]->push(e);
    cpu_queues.mine().lock.unlock();
}

/**
 * for testing: creates an event to run on a specific core
 * **note** not race condition safe
 */
inline void create_event_core(Function<void()> w, int priority, int core) {
    event* e = new event_work(w);
    cpu_queues.forCPU(core).lock.lock();
    cpu_queues.forCPU(core).queue_list[priority]->push(e);
    cpu_queues.forCPU(core).lock.unlock();
}

extern event* pop(int cpu);
void loop();

#endif