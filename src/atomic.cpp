#include "atomic.h"

#include "event.h"
#include "printf.h"
#include "queue.h"

void Semaphore::up() {
    LockGuard<SpinLock> guard(spin_lock);
    if (blocked_queue.get_size() > 0) {
        SemaphoreNode* node = blocked_queue.remove();
        create_event(node->work);
        delete node;
    } else {
        value++;
    }
}

void Semaphore::down(Function<void()> w) {
    LockGuard<SpinLock> guard(spin_lock);
    if (value > 0) {
        create_event(w);
        value--;
    } else {
        SemaphoreNode* node = new SemaphoreNode(w);
        blocked_queue.add(node);
    }
}