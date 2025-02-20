#ifndef _threads_h_
#define _threads_h_

#include "atomic.h"
#include "queue.h"
#include "heap.h"
#include "printf.h"
#define CORE_COUNT 4
#define THREAD_CPU_CONTEXT 0

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
        cpu_context context;
        TCB *next;
        uint64_t *saved_SP;
        bool wasDisabled = false;
        Atomic<int> done{false};

        virtual void doTheWork() = 0; // Abstract/virtual function that must be overridden
        virtual ~TCB() {};            // Allows child classes to be deleted
    };

    extern void block(Queue<TCB, SpinLock> *q, /*ISL*/ SpinLock *isl); // generalized yield
    extern void entry();                                               // have a thread start work
    extern void restoreState();                                        // restore state post context switch
    extern void clearZombies();                                        // delete threads that have called stop.

    template <typename Work>
    struct TCBWithWork : public TCB
    {
        Work work;
        alignas(16) uint64_t stack[2048]; // Ensure proper 16-byte alignment
        // uint64_t stack[2048];
        TCBWithWork(Work work) : work(work)
        {
            printf("stack addr: %x\n", (uint64_t)&stack[2048]);
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

            auto bottomIndex = 2048 - 1;
            // bottomIndex &= ~1; // Ensure 16-byte alignment
            // stack[bottomIndex] = 0;                    // x19
            // stack[bottomIndex - 1] = 0;                // x20
            // stack[bottomIndex - 2] = 0;                // x21
            // stack[bottomIndex - 3] = 0;                // x22
            // stack[bottomIndex - 4] = 0;                // x23
            // stack[bottomIndex - 5] = 0;                // x24
            // stack[bottomIndex - 6] = 0;                // x25
            // stack[bottomIndex - 7] = 0;                // x26
            // stack[bottomIndex - 8] = 0;                // x27
            // stack[bottomIndex - 9] = 0;                // x28
            // stack[bottomIndex - 10] = 0;               // x29 fp
            // stack[bottomIndex - 11] = (uint64_t)entry; // x30 lr
            // saved_SP = &stack[bottomIndex - 12];       // sp
            // state is what the thread is doing
            done.set(false);
        }

        void doTheWork() override
        {
            work();
        }
    };

    struct IdleThread : public TCB
    {
        void doTheWork() override
        {
            while (true)
                ;
        }
    };
    extern Queue<TCB, SpinLock> readyQ;
    extern Queue<TCB, SpinLock> zombieQ;
};

template <typename T>
void thread(T work)
{
    // using namespace alogx;
    alogx::clearZombies();
    auto tcb = new alogx::TCBWithWork(work);
    alogx::readyQ.add(tcb);
}

#endif
