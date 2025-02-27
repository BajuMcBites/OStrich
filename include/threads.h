#ifndef _threads_h_
#define _threads_h_
#include "atomic.h"
#include "queue.h"
#include "heap.h"
#include "printf.h"
#define CORE_COUNT 4
#define THREAD_CPU_CONTEXT 0

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

    extern void block(LockedQueue<TCB, SpinLock> *q, /*ISL*/ SpinLock *isl); // generalized yield
    extern void entry();                                                     // have a thread start work
    extern void restoreState();                                              // restore state post context switch
    extern void clearZombies();                                              // delete threads that have called stop.

    template <typename Work>
    struct UserTCB : public TCB
    {
        cpu_context context;
        Work work;
        alignas(16) uint64_t stack[2048]; // Ensure proper 16-byte alignment
        // uint64_t stack[2048];
        UserTCB(Work work) : work(work)
        {
            // printf("stack addr: %x\n", (uint64_t)&stack[2048]);
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
            work();
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

template <typename T>
void user_thread(T work)
{
    // using namespace alogx;
    alogx::clearZombies();
    auto tcb = new alogx::UserTCB(work);
    alogx::readyQ.add(tcb);
}

#endif