
#include "stdint.h"
#include "threads.h"
#include "queue.h"
#include "atomic.h"
#include "printf.h"
#include "utils.h"
#include "libk.h"
#include "event.h"
// ------------
// threads.cc -
// ------------

extern "C" void context_switch(uint64_t **saveOldSpHere, uint64_t *newSP);
extern "C" void cpu_switch_to(alogx::cpu_context *prevTCB, alogx::cpu_context *nextTCB);
// global ready queue
namespace alogx
{
    LockedQueue<TCB, SpinLock> readyQ{};         // Queue of threads ready to run
    LockedQueue<TCB, SpinLock> zombieQ{};        // Queue of threads ready to be deleted
    TCB *runningThreads[CORE_COUNT] = {nullptr}; // use an array for multiple cores, index with SMP::me()
    TCB *oldThreads[CORE_COUNT] = {nullptr};     // save the oldThread before context switching
    cpu_context *coreContext[CORE_COUNT] = {nullptr};
    LockedQueue<TCB, SpinLock> *addMeHere[CORE_COUNT]; // save the queue I want to be added to before context switching
    /*ISL*/ SpinLock *myLock[CORE_COUNT];              // save the isl of the thread
    bool oldState[CORE_COUNT];                         // save interrupt state

    struct DummyThread : public TCB
    {
        DummyThread()
        {
            done.set(true);
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
        // printf("Thread %p is starting (SP: %p)\n", getCoreID(), alogx::runningThreads[getCoreID()]->saved_SP);
        auto me = getCoreID();
        K::assert(runningThreads[me] != nullptr, "null pointer in entry");
        auto thread = runningThreads[me];
        thread->done.set(false);
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
        runningThreads[i]->done.set(true);
        coreContext[i] = new cpu_context();
        // set sp or do I not need to. for the initial dummies
        // coreContext[i]->??;
    }
}

void yield()
{
    alogx::block(&alogx::readyQ, nullptr);
}

void stop()
{
    using namespace alogx;
    clearZombies();
    // printf("cleared zombies\n");
    while (true)
    {
        block(&zombieQ, nullptr);
    }
    // PANIC("Stop returning OH NO \n");
}

namespace alogx
{
    void block(LockedQueue<TCB, SpinLock> *q, /*ISL*/ SpinLock *isl)
    {
        // printf("attempting to block\n");
        using namespace alogx;
        // bool was = Interrupts::disable();
        auto nextThread = alogx::readyQ.remove();
        if (nextThread == nullptr) // no other threads. I can keep working/spinning
        {
            if (isl)
            {
                isl->unlock();
            }
            // Interrupts::restore(was);
            return;
        }
        int me = getCoreID();
        K::assert(alogx::runningThreads[me] != nullptr, "null pointer in block");

        oldThreads[me] = runningThreads[me]; // save the old thread
        // oldThreads[me]->wasDisabled = was;   // save the interrupt state prior to block
        // oldState[me] = was;                  // save the interrupt state prior to block
        addMeHere[me] = q; // save the queue to add me on
        myLock[me] = isl;  // save my lock

        alogx::runningThreads[me] = nextThread; // This is a lie, only observable by the core. its ok ?
        // cases: kernel to user, user to kernel, kernel to kernel. user to user
        if (oldThreads[me]->kernel_event && nextThread->kernel_event) // kernel to kernel
        {
            cpu_switch_to(coreContext[me], coreContext[me]);
        }
        else if (!oldThreads[me]->kernel_event && !nextThread->kernel_event) // user to user
        {
            printf("user to user\n");
            cpu_switch_to(&((UserTCB<decltype(nextThread)> *)oldThreads[me])->context,
                          &((UserTCB<decltype(nextThread)> *)nextThread)->context);
        }
        else if (!oldThreads[me]->kernel_event && nextThread->kernel_event) // user to kernel
        {
            printf("user to kernel\n");
            cpu_switch_to(&((UserTCB<decltype(nextThread)> *)oldThreads[me])->context,
                          coreContext[me]);
        }
        else // kernel to user
        {
            printf("kernel to user\n");
            cpu_switch_to(coreContext[me],
                          &((UserTCB<decltype(nextThread)> *)nextThread)->context);
        }

        // context_switch(&oldThreads[me]->saved_SP, nextThread->saved_SP);
        // printf("did I make it?\n");
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

    void clearZombies()
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
