#include "atomic.h"

#include "event.h"
#include "printf.h"
#include "queue.h"
#include "sched.h"

void Semaphore::up() {
    LockGuard<SpinLock> guard(spin_lock);
    value++;
    if (value > 0 && blocked_queue.get_size() > 0) {
        value--;
        SemaphoreNode* node = blocked_queue.remove();
        create_event(node->work);
        delete node;
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

extern "C" void load_kernel_context(core_context* context);

void BlockingSemaphore::up() {
    LockGuard<SpinLock> lock(this->spin_lock);
    if (this->blocked_queue.get_size() > 0) {
        core_context* frame = this->blocked_queue.remove()->frame;
        create_event_core([=]() { load_kernel_context(frame); }, frame->id);
    }
}

void save_context(core_context* context) {
    asm volatile("mov %0, x0" : "=r"(context->x0));
    asm volatile("mov %0, x1" : "=r"(context->x1));
    asm volatile("mov %0, x2" : "=r"(context->x2));
    asm volatile("mov %0, x3" : "=r"(context->x3));
    asm volatile("mov %0, x4" : "=r"(context->x4));
    asm volatile("mov %0, x5" : "=r"(context->x5));
    asm volatile("mov %0, x6" : "=r"(context->x6));
    asm volatile("mov %0, x7" : "=r"(context->x7));
    asm volatile("mov %0, x8" : "=r"(context->x8));
    asm volatile("mov %0, x9" : "=r"(context->x9));
    asm volatile("mov %0, x10" : "=r"(context->x10));
    asm volatile("mov %0, x11" : "=r"(context->x11));
    asm volatile("mov %0, x12" : "=r"(context->x12));
    asm volatile("mov %0, x13" : "=r"(context->x13));
    asm volatile("mov %0, x14" : "=r"(context->x14));
    asm volatile("mov %0, x15" : "=r"(context->x15));
    asm volatile("mov %0, x16" : "=r"(context->x16));
    asm volatile("mov %0, x17" : "=r"(context->x17));
    asm volatile("mov %0, x18" : "=r"(context->x18));
    asm volatile("mov %0, x19" : "=r"(context->x19));
    asm volatile("mov %0, x20" : "=r"(context->x20));
    asm volatile("mov %0, x21" : "=r"(context->x21));
    asm volatile("mov %0, x22" : "=r"(context->x22));
    asm volatile("mov %0, x23" : "=r"(context->x23));
    asm volatile("mov %0, x24" : "=r"(context->x24));
    asm volatile("mov %0, x25" : "=r"(context->x25));
    asm volatile("mov %0, x26" : "=r"(context->x26));
    asm volatile("mov %0, x27" : "=r"(context->x27));
    asm volatile("mov %0, x28" : "=r"(context->x28));
    asm volatile("mov %0, x29" : "=r"(context->x29));
    asm volatile("mov %0, x30" : "=r"(context->x30));
    asm volatile("mrs %0, sp_el0" : "=r"(context->sp));
    asm volatile("mrs %0, elr_el1" : "=r"(context->pc));
    asm volatile("mrs %0, spsr_el1" : "=r"(context->spsr));
    asm volatile("mrs %0, mpidr_el1" : "=r"(context->id));
    context->id &= 0xFF;
    return;
}

void BlockingSemaphore::down() {
    this->spin_lock.lock();
    if (value > 0) {
        value--;
        this->spin_lock.unlock();
    } else {
        bool restored = false;
        core_context* context = new core_context();
        save_context(context);

        if (restored) {
            printf("BlockingSemaphore::down() I'M ALIVE!!\n");
            return;
        }
        restored = true;

        this->blocked_queue.add(new BlockingNode(context));
        this->spin_lock.unlock();
        kernel_yield();
    }
}
