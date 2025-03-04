#include "atomic.h"
#include "event_loop.h"
#include "printf.h"

//TODO: need to make an event queue instead of Function queue since you cannot copy Functions
void Semaphore::up() {
    LockGuard<SpinLock> guard(spin_lock);
    if (!blocking_queue.empty()) {
        printf("Called UP\n");
        Function<void()>* w = blocking_queue.top();
        blocking_queue.pop();
        printf("HERE%d\n", w);

        create_event(*w, 1);
    } else {
        value++;
    }
}

void Semaphore::down(Function<void()> w) {
    LockGuard<SpinLock> guard(spin_lock);
    if (value > 0) {
        create_event(w, 1);
        value--;
    } else {
        printf("BLOCKING %d\n", &w);
        blocking_queue.push(&w);
    }
}