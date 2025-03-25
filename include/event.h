#ifndef _threads_h_
#define _threads_h_
#include "atomic.h"
#include "function.h"
#include "heap.h"
#include "locked_queue.h"
#include "percpu.h"
#include "printf.h"
#include "vm.h"
#define CORE_COUNT 4
#define THREAD_CPU_CONTEXT 0
#define PRIORITY_LEVELS 5

//-------------
//--event.h--
//-------------

extern void threadsInit();

extern void stop();
extern void yield();

struct cpu_context {
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
    unsigned long int fp;
    unsigned long int sp;
    unsigned long int pc;
};

struct TCB {
    TCB* next;
    bool wasDisabled = false;  // previous interrupt state
    bool kernel_event = true;
    virtual void run() = 0;  // Abstract/virtual function that must be overridden
    virtual ~TCB() {};       // Allows child classes to be deleted
};
struct CPU_Queues {
    LockedQueue<TCB, SpinLock> queues[PRIORITY_LEVELS];
};

extern PerCPU<CPU_Queues> readyQueue;

extern void event_loop(LockedQueue<TCB, SpinLock>* q, /*ISL*/ SpinLock* isl);  // generalized yield
extern void entry();         // have a thread start work
extern void restoreState();  // restore state post context switch
extern void clearZombies();  // delete threads that have called stop.

// template <typename Work>
struct UserTCB : public TCB {
    cpu_context context;
    PageTable* page_table;
    SupplementalPageTable* supp_page_table;
    Function<void()> w;
    alignas(16) uint64_t stack[2048];  // Ensure proper 16-byte alignment
    template <typename lambda>
    UserTCB(lambda w) : w(w) {
        page_table = new PageTable;
        supp_page_table = new SupplementalPageTable;

        context.x19 = 0;
        context.x20 = 0;
        context.x21 = 0;
        context.x22 = 0;
        context.x23 = 0;
        context.x24 = 0;
        context.x25 = 0;
        context.x26 = 0;
        context.x27 = 0;
        context.x28 = 0;
        context.sp = (uint64_t)&stack[2048];  // Stack grows downward
        context.pc = (uint64_t)&entry;        // Function to execute when the thread starts
        kernel_event = false;
    }

    ~UserTCB() {
        delete page_table;
        delete supp_page_table;
    }

    void run() override {
        w();
    }
};

template <typename T>
struct UserValue : public TCB {
    cpu_context context;
    PageTable* page_table;
    SupplementalPageTable* supp_page_table;
    Function<void(T)> w;
    T value;
    alignas(16) uint64_t stack[2048];  // Ensure proper 16-byte alignment
    template <typename lambda>
    UserValue(lambda w, T value) : w(w), value(value) {
        page_table = new PageTable;
        supp_page_table = new SupplementalPageTable;

        context.x19 = 0;
        context.x20 = 0;
        context.x21 = 0;
        context.x22 = 0;
        context.x23 = 0;
        context.x24 = 0;
        context.x25 = 0;
        context.x26 = 0;
        context.x27 = 0;
        context.x28 = 0;
        context.sp = (uint64_t)&stack[2048];  // Stack grows downward
        context.pc = (uint64_t)&entry;        // Function to execute when the thread starts
        kernel_event = false;
    }

    ~UserValue() {
        delete page_table;
        delete supp_page_table;
    }

    void run() override {
        w(value);
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

extern LockedQueue<TCB, SpinLock> zombieQ;

template <typename lambda>
inline void user_thread(lambda work) {
    auto tcb = new UserTCB(work);
    readyQueue.mine().queues[1].add(tcb);
}

template <typename T>
inline void user_thread(Function<void(T)> work, T value) {
    auto tcb = new UserValue<T>(work, value);
    readyQueue.mine().queues[1].add(tcb);
}

template <typename lambda>
inline void user_thread(lambda work, int priority) {
    auto tcb = new UserTCB(work);
    readyQueue.mine().queues[priority].add(tcb);
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

TCB* currentTCB(int core);

#endif