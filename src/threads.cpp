
#include "stdint.h"
#include "threads.h"
#include "queue.h"
#include "atomic.h"
#include "printf.h"
#include "utils.h"
#include "libk.h"
#include "event.h"
#include "percpu.h"
// ------------
// threads.cc -
// ------------

extern "C" void context_switch(uint64_t **saveOldSpHere, uint64_t *newSP);
extern "C" void cpu_switch_to(alogx::cpu_context *prevTCB, alogx::cpu_context *nextTCB);
// global ready queue

// PerCPU<alogx::CPU_Queues> readyQueue;

namespace alogx
{
    PerCPU<alogx::CPU_Queues> readyQueue;
    LockedQueue<TCB, SpinLock> readyQ{};         // Queue of threads ready to run
    LockedQueue<TCB, SpinLock> zombieQ{};        // Queue of threads ready to be deleted
    TCB *runningThreads[CORE_COUNT] = {nullptr}; // use an array for multiple cores, index with SMP::me()
    TCB *oldThreads[CORE_COUNT] = {nullptr};     // save the oldThread before context switching
    cpu_context *coreContext[CORE_COUNT] = {nullptr};
    LockedQueue<TCB, SpinLock> *addMeHere[CORE_COUNT]; // save the queue I want to be added to before context switching
    /*ISL*/ SpinLock *myLock[CORE_COUNT];              // save the isl of the thread
    bool oldState[CORE_COUNT];                         // save interrupt state

    TCB *getNextEvent(int core)
    {
        auto &ready = readyQueue.forCPU(core);

        for (int i = 0; i < PRIORITY_LEVELS; i++)
        {
            auto next = ready.queues[i].remove();
            if (next)
            {
                return next;
            }
        }
        return nullptr;
    }

    struct DummyThread : public TCB
    {
        DummyThread()
        {
            kernel_event = true;
        }
        void run() override
        {
            // PANIC("should never go here");
        }
    };

    void entry()
    {
        // ASSERT(Interrupts::isDisabled());
        auto me = getCoreID();
        K::assert(runningThreads[me] != nullptr, "null pointer in entry");
        auto thread = runningThreads[me];
        thread->wasDisabled = false;
        restoreState();
        thread->run();
        stop();
    }
};

void threadsInit() // place to put any intialization logic
{
    using namespace alogx;
    for (int i = 0; i < CORE_COUNT; i++)
    {
        runningThreads[i] = new DummyThread();
        coreContext[i] = new cpu_context();
        // set sp or do I not need to. for the initial dummies
        // coreContext[i]->??;
    }
}

void yield()
{
    // Get the current core's ready queue
    auto &ready = alogx::readyQueue.forCPU(getCoreID()); // Use getCoreID() for per-core access

    // Retrieve the queue with a specific priority (1 in this case)
    auto theQueue = &ready.queues[1]; // Use address of the queue

    alogx::event_loop(theQueue, nullptr);
}

void stop()
{
    using namespace alogx;
    clearZombies();
    while (true)
    {
        event_loop(&zombieQ, nullptr);
    }
    // PANIC("Should not go here. We should be in Event Loop\n");
}

namespace alogx
{
    void event_loop(LockedQueue<TCB, SpinLock> *q, /*ISL*/ SpinLock *isl)
    {
        using namespace alogx;
        int me = getCoreID();
        // bool was = Interrupts::disable();
        auto nextThread = getNextEvent(me);
        if (nextThread == nullptr) // try to steal work
        {
            int nextCore = (me + 1) % CORE_COUNT;
            while (nextCore != me && nextThread == nullptr)
            {
                nextThread = getNextEvent(nextCore);
                nextCore = (nextCore + 1) % CORE_COUNT; // Move to the next core
            }
            if (nextThread == nullptr) // no other threads. I can keep working/spinning
            {
                if (isl)
                {
                    isl->unlock();
                }
                // Interrupts::restore(was);
                return;
            }
        }
        K::assert(alogx::runningThreads[me] != nullptr, "null pointer trying to run as event");

        oldThreads[me] = runningThreads[me]; // save the old thread
        // oldThreads[me]->wasDisabled = was;   // save the interrupt state prior to block
        // oldState[me] = was;                  // save the interrupt state prior to block
        addMeHere[me] = q; // save the queue to add me on
        myLock[me] = isl;  // save my lock

        alogx::runningThreads[me] = nextThread; // This is a lie, only observable by the core. its ok ?

        // cases: kernel to user, user to kernel, kernel to kernel. user to user
        if (oldThreads[me]->kernel_event && nextThread->kernel_event) // kernel to kernel
        {
            coreContext[me]->pc = (uint64_t)&entry; // Function to execute when the thread starts
            entry();
        }
        else if (!oldThreads[me]->kernel_event && !nextThread->kernel_event) // user to user
        {
            cpu_switch_to(&((UserTCB *)oldThreads[me])->context,
                          &((UserTCB *)nextThread)->context);
        }
        else if (!oldThreads[me]->kernel_event && nextThread->kernel_event) // user to kernel
        {
            coreContext[me]->pc = (uint64_t)&entry; // Function to
            cpu_switch_to(&((UserTCB *)oldThreads[me])->context,
                          coreContext[me]);
            entry();
        }
        else // kernel to user
        {
            cpu_switch_to(coreContext[me],
                          &((UserTCB *)nextThread)->context);
        }
        // ASSERT(Interrupts::isDisabled());
        restoreState();
    }

    void restoreState()
    {
        // ASSERT(Interrupts::isDisabled());
        int me = getCoreID();
        auto prev = oldThreads[me];
        // auto running = runningThreads[me];
        if (prev)
        {
            if (addMeHere[me] != nullptr)
            {
                addMeHere[me]->add(prev);
            }
            // I have successfully added my prev to desired queue,
            // I can unlock the lock if I was given one
            if (myLock[me] != nullptr)
            {
                myLock[me]->unlock();
                // Interrupts::restore(oldThreads[me]->wasDisabled);
            }
            // Interrupts::restore(prev->wasDisabled);
        }

        // if (running->wasDisabled)
        // {
        //     cli(); // disable
        // }
        // else
        // {
        //     Interrupts::restore(running->wasDisabled);
        // }
    }

    void clearZombies() // function to delete completed events
    {
        auto killMe = zombieQ.remove();
        while (killMe)
        {
            // bool was = Interrupts::disable();
            delete killMe;
            // Interrupts::restore(was);
            killMe = zombieQ.remove();
        }
    }
};
