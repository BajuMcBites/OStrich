#include "queue.h"
#include "heap.h"
#include "event_loop.h"
#include "printf.h"

PerCPU<percpu_queue> cpu_queues;

event *pop(int cpu)
{
    auto &q = cpu_queues.forCPU(cpu);
    LockGuard<SpinLock> guard(q.lock);
    for (int i = 0; i < MAX_PRIORITY; i++)
    {
        if (!q.queue_list[i]->empty())
        {
            event *e = q.queue_list[i]->top();
            q.queue_list[i]->pop();
            return e;
        }
    }
    return nullptr;
}

// alogx::TCB *popTCB(int cpu)
// {
//     percpu_queue &q = cpu_queues.forCPU(cpu);
//     for (int i = 0; i < MAX_PRIORITY; i++)
//     {
//         alogx::TCB **tcb = q.queues[i]->remove();
//         if (tcb)
//         {
//             return *tcb;
//         }
//     }
//     return nullptr;
// }

void loop()
{

    event *curr = pop(getCoreID());
    if (curr)
    {
        curr->run();
        return;
    }
    for (int i = 0; i < 4; i++)
    {
        curr = nullptr;
        if (curr = pop(i))
        {
            curr->run();
        }
    }
}