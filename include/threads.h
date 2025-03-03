#ifndef _threads_h_
#define _threads_h_
#include "atomic.h"
#include "queue.h"
#include "heap.h"
#include "printf.h"
#include "function.h"
#include "percpu.h"
#define CORE_COUNT 4
#define THREAD_CPU_CONTEXT 0
#define PRIORITY_LEVELS 5

//-------------
//--threads.h--
//-------------

extern void threadsInit();

extern void stop();
extern void yield();
namespace alogx
{

    struct cpu_context
    {
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

    struct TCB
    {
        // cpu_context context;
        TCB *next;
        bool wasDisabled = false; // previous interrupt state
        Atomic<int> done{false};  // is tihs work complete
        bool kernel_event = true;
        virtual void run() = 0; // Abstract/virtual function that must be overridden
        virtual ~TCB() {};      // Allows child classes to be deleted
    };

    struct CPU_Queues
    {
        LockedQueue<alogx::TCB, SpinLock> queues[PRIORITY_LEVELS];
    };

    extern PerCPU<CPU_Queues> readyQueue;

    extern void event_loop(LockedQueue<TCB, SpinLock> *q, /*ISL*/ SpinLock *isl); // generalized yield
    extern void entry();                                                          // have a thread start work
    extern void restoreState();                                                   // restore state post context switch
    extern void clearZombies();                                                   // delete threads that have called stop.

    // template <typename Work>
    struct UserTCB : public TCB
    {
        cpu_context context;
        Function<void()> w;
        // Work work;
        alignas(16) uint64_t stack[2048]; // Ensure proper 16-byte alignment
        // uint64_t stack[2048];
        template <typename lambda>
        UserTCB(lambda w) : w(w)
        {
            // this->work = Function<void()>(work); // Store the work as a Function
            //  printf("stack addr: %x\n", (uint64_t)&stack[2048]);
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
            context.sp = (uint64_t)&stack[2048]; // Stack grows downward
            context.pc = (uint64_t)&entry;       // Function to execute when the thread starts
            done.set(false);
            kernel_event = false;
        }

        void run() override
        {

            w();
        }
    };

    // template <typename Work>
    struct Event : public TCB
    {
        Function<void()> w;
        // Work work;
        template <typename lambda>
        Event(lambda w) : w(w)
        {
            // this->work = Function<void()>(work); // Store the work as a Function
            done.set(false);
            kernel_event = true;
        }

        void run() override
        {
            w();
        }
    };

    template <typename T>
    struct EventValue : public TCB
    {
        Function<void(T)> w;
        T value;
        //  Work work;
        template <typename lambda>
        EventValue(lambda w, T value) : w(w), value(value)
        {
            kernel_event = true;
        }

        void run() override
        {
            w(value);
        }
    };

    struct IdleThread : public TCB
    {
        void run() override
        {
            while (true)
                ;
        }
    };
    extern LockedQueue<TCB, SpinLock> readyQ;
    extern LockedQueue<TCB, SpinLock> zombieQ;
};

template <typename lambda>
inline void user_thread(lambda work)
{
    using namespace alogx;
    // alogx::clearZombies();
    auto tcb = new UserTCB(work);
    readyQueue.mine().queues[1].add(tcb);
}

template <typename lambda>
inline void create_kernel_event(lambda work, int priority)
{
    auto tcb = new alogx::Event(work);
    alogx::readyQueue.mine().queues[priority].add(tcb);
}

template <typename T>
inline void create_kernel_event(Function<void(T)> work, T value, int priority) // lambda that captures values
{
    auto tcb = new alogx::EventValue<T>(work, value);
    alogx::readyQueue.mine().queues[priority].add(tcb);
}

template <typename lambda>
inline void create_kernel_event(lambda work)
{
    auto tcb = new alogx::Event(work);
    alogx::readyQueue.mine().queues[1].add(tcb);
}

template <typename T>
inline void create_kernel_event(Function<void(T)> work, T value) // lambda that captures values
{
    auto tcb = new alogx::EventValue<T>(work, value);
    alogx::readyQueue.mine().queues[1].add(tcb);
}
#endif