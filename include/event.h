#ifndef _threads_h_
#define _threads_h_
#include "atomic.h"
#include "function.h"
#include "heap.h"
#include "locked_queue.h"
#include "percpu.h"
#include "printf.h"
#include "process.h"
#include "vm.h"
#define CORE_COUNT 4
#define THREAD_CPU_CONTEXT 0
#define PRIORITY_LEVELS 5

//-------------
// --event.h--
//-------------

struct cpu_context {
    unsigned long int x0;
    unsigned long int x1;
    unsigned long int x2;
    unsigned long int x3;
    unsigned long int x4;
    unsigned long int x5;
    unsigned long int x6;
    unsigned long int x7;
    unsigned long int x8;
    unsigned long int x9;
    unsigned long int x10;
    unsigned long int x11;
    unsigned long int x12;
    unsigned long int x13;
    unsigned long int x14;
    unsigned long int x15;
    unsigned long int x16;
    unsigned long int x17;
    unsigned long int x18;
    unsigned long int x19;
    unsigned long int x20;
    unsigned long int x21;
    unsigned long int x22;
    unsigned long int x23;
    unsigned long int x24;
    unsigned long int x25;
    unsigned long int x26;
    unsigned long int x27;
    unsigned long int x28;
    unsigned long int x29;
    unsigned long int x30;
    unsigned long int sp;
    unsigned long int pc;
    unsigned long int spsr;
};

struct TCB {
    TCB* next = nullptr;
    bool kernel_event = true;
    virtual void run() = 0;  // Abstract/virtual function that must be overridden
    virtual ~TCB() {};       // Allows child classes to be deleted
};
struct CPU_Queues {
    LockedQueue<TCB, SpinLock> queues[PRIORITY_LEVELS];
};

extern PerCPU<CPU_Queues> readyQueue;

extern void event_loop();
extern void enter_user_space(struct UserTCB* tcb);
extern void save_user_context(struct UserTCB* tcb, uint64_t* regs);

struct UserTCB : public TCB {
    cpu_context context;
    PCB* pcb = nullptr;
    Function<void(UserTCB*)> w;
    UserTCB() : w(enter_user_space) {
        context.spsr = 0;
        context.sp = 0;
        context.pc = 0;
        kernel_event = false;
    }

    void run() override {
        w(this);
    }
};

struct Event : public TCB {
    Function<void()> w;
    template <typename lambda>
    Event(lambda w) : w(w) {
        kernel_event = true;
    }

    void run() override {
        w();
    }
};

template <typename T>
struct EventValue : public TCB {
    Function<void(T)> w;
    T value;
    template <typename lambda>
    EventValue(lambda w, T value) : w(w), value(value) {
        kernel_event = true;
    }

    void run() override {
        w(value);
    }
};

inline void queue_user_tcb(UserTCB* tcb, int priority) {
    readyQueue.mine().queues[priority].add(tcb);
}

inline void queue_user_tcb(UserTCB* tcb) {
    readyQueue.mine().queues[1].add(tcb);
}

template <typename lambda>
inline void create_event(lambda work) {
    auto tcb = new Event(work);
    readyQueue.mine().queues[1].add(tcb);
}

template <typename lambda>
inline void create_event(lambda work, int priority) {
    auto tcb = new Event(work);
    readyQueue.mine().queues[priority].add(tcb);
}

template <typename T>
inline void create_event(Function<void(T)> work,
                         T value)  // lambda that captures values
{
    auto tcb = new EventValue<T>(work, value);
    readyQueue.mine().queues[1].add(tcb);
}

template <typename T>
inline void create_event(Function<void(T)> work, T value,
                         int priority)  // lambda that captures values
{
    auto tcb = new EventValue<T>(work, value);
    readyQueue.mine().queues[priority].add(tcb);
}

inline void create_event_core(
    Function<void()> work,
    int core)  // Queues work on a deticated core (used for testing semaphores)
{
    auto tcb = new Event(work);
    readyQueue.forCPU(core).queues[1].add(tcb);
}

void set_return_value(UserTCB* tcb, uint64_t ret_val);

UserTCB* get_running_user_tcb(int core);

#endif