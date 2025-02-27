#include "stdint.h"
#include "threads.h"
#include "queue.h"
#include "atomic.h"
#include "printf.h"
#include "utils.h"
#include "libk.h"
// ------------
// threads.cc -
// ------------

extern "C" void context_switch(uint64_t **saveOldSpHere, uint64_t *newSP);
extern "C" void cpu_switch_to(alogx::cpu_context *prevTCB, alogx::cpu_context *nextTCB);
// global ready queue
namespace alogx
{
    LockedQueue<TCB, SpinLock> readyQ{};               // Queue of threads ready to run
    LockedQueue<TCB, SpinLock> zombieQ{};              // Queue of threads ready to be deleted
    TCB *runningThreads[CORE_COUNT] = {nullptr};       // use an array for multiple cores, index with SMP::me()
    TCB *oldThreads[CORE_COUNT] = {nullptr};           // save the oldThread before context switching
    LockedQueue<TCB, SpinLock> *addMeHere[CORE_COUNT]; // save the queue I want to be added to before context switching
    /*ISL*/ SpinLock *myLock[CORE_COUNT];              // save the isl of the thread
    bool oldState[CORE_COUNT];                         // save interrupt state

    struct DummyThread : public TCB
    {
        void doTheWork() override
        {
            // PANIC("should never go here");
        }
    };

    void entry()
    {
        // ASSERT(Interrupts::isDisabled());
        // printf("Thread %p is starting (SP: %p)\n", getCoreID(), alogx::runningThreads[getCoreID()]->saved_SP);
        auto me = getCoreID();
        // ASSERT(runningThreads[me] != nullptr);
        auto thread = runningThreads[me];
        thread->done.set(false);
        thread->wasDisabled = false;
        restoreState();
        thread->doTheWork();
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
        // printf("core %d found a thread!\n", me);
        // ASSERT(alogx::runningThreads[me] != nullptr);

        oldThreads[me] = runningThreads[me]; // save the old thread
        // oldThreads[me]->wasDisabled = was;   // save the interrupt state prior to block
        // oldState[me] = was;                  // save the interrupt state prior to block
        addMeHere[me] = q; // save the queue to add me on
        myLock[me] = isl;  // save my lock

        alogx::runningThreads[me] = nextThread; // This is a lie, only observable by the core. its ok ?

        // printf("context swtich attempt\n");
        // printf("My SP %x\n", oldThreads[me]->context.sp);
        // printf("Next SP %x\n", nextThread->context.sp);
        // printf("Next SP ADDR %x\n", &nextThread->context.sp);

        // for (int i = 0; i <= 11; ++i)
        // {
        //     printf("x%d: %x\n", i + 19, *((uint64_t *)nextThread + i));
        //     printf("x%d:  ADDR: %x\n", i + 19, (uint64_t *)(nextThread) + i);
        // }
        // printf("my SP %x\n", &oldThreads[me]->saved_SP);
        // printf("next SP %x\n", nextThread->saved_SP);
        cpu_switch_to(&oldThreads[me]->context, &nextThread->context);
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