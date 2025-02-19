#ifndef _threads_h_
#define _threads_h_

#include "atomic.h"
#include "queue.h"
#include "heap.h"

#define CORE_COUNT 4

extern void threadsInit();

extern void stop();
extern void yield();
namespace alogx
{

    struct TCB
    {
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

            auto bottomIndex = 2048 - 1;
            bottomIndex &= ~1; // Ensure 16-byte alignment
            stack[bottomIndex] = 0;
            stack[bottomIndex - 1] = 0;
            stack[bottomIndex - 2] = 0;
            stack[bottomIndex - 3] = 0;
            stack[bottomIndex - 4] = 0;
            stack[bottomIndex - 5] = 0;
            stack[bottomIndex - 6] = 0;
            stack[bottomIndex - 7] = 0;
            stack[bottomIndex - 8] = 0;
            stack[bottomIndex - 9] = 0;
            stack[bottomIndex - 10] = 0;
            stack[bottomIndex - 11] = (uint64_t)entry; // return address in x30
                                                       // stack[bottomIndex - 12] = (uint64_t)entry; // return address in x30

            saved_SP = &stack[bottomIndex - 12];
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
