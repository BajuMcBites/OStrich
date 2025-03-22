
#include "event.h"

#include "atomic.h"
#include "libk.h"
#include "percpu.h"
#include "printf.h"
#include "queue.h"
#include "stdint.h"
#include "utils.h"
// ------------
// event.cpp --
// ------------

extern "C" void cpu_switch_to(cpu_context *prevTCB, cpu_context *nextTCB);
extern "C" void load_context(cpu_context *prevTCB, cpu_context *nextTCB);
extern "C" void save_context(cpu_context *prevTCB, cpu_context *nextTCB);

// queue of TCBs ready to run
PerCPU<CPU_Queues> readyQueue;

// Queue of threads ready to be deleted
LockedQueue<TCB, SpinLock> zombieQ{};  // Queue of threads ready to be deleted

// use an array for multiple cores, index with getCoreId
TCB *runningThreads[CORE_COUNT] = {nullptr};

// save the oldThread before context switching
TCB *oldThreads[CORE_COUNT] = {nullptr};

// cpu context for events per core. will be needed for interrupts
cpu_context *coreContext[CORE_COUNT] = {nullptr};

// save the queue I want to be added to before context change
LockedQueue<TCB, SpinLock> *addMeHere[CORE_COUNT];

// save the isl of the thread
/*ISL*/ SpinLock *myLock[CORE_COUNT];

// save interrupt state. not sure if this is needed. I think it will be for handling interrupts
bool oldState[CORE_COUNT];

// get next event off of the ready queue
TCB *getNextEvent(int core) {
    auto &ready = readyQueue.forCPU(core);
    for (int i = 0; i < PRIORITY_LEVELS; i++) {
        auto next = ready.queues[i].remove();
        if (next != nullptr) {
            return next;
        }
    }
    return nullptr;
}

// temp struct to fill into oldThreads so that we have something to switch from for initial context
// switch
struct DummyThread : public TCB {
    DummyThread() {
        kernel_event = true;
    }
    void run() override {
        // PANIC("should never go here");
    }
};

// function to call run and initialize state before starting a task
void entry() {
    // ASSERT(Interrupts::isDisabled());
    auto me = getCoreID();
    auto thread = runningThreads[me];
    K::assert(thread != nullptr, "null pointer in entry");
    thread->wasDisabled = false;
    restoreState();
    thread->run();
    stop();
}

// place to put any intialization logic for events
void threadsInit() {
    for (int i = 0; i < CORE_COUNT; i++) {
        runningThreads[i] = new DummyThread();
        coreContext[i] = new cpu_context();
    }
}

// forces an event to change context.
void yield() {
    auto me = getCoreID();
    // safety. a kernel event should not be forced to yield.
    if (runningThreads[me]->kernel_event) {
        // if handling an interrupt maybe create a function scheduleInterrupt().
        // you context switch to interrupt context and stack, schedule interrupt event, switch back
        // to kernel event.
        return;
    }
    // Get the current core's ready queue
    // per-core access
    auto &ready = readyQueue.forCPU(getCoreID());  // Use getCoreID() for per-core access

    // Retrieve the queue with a specific priority (1 in this case)
    auto theQueue = &ready.queues[1];  // Use address of the queue

    event_loop(theQueue, nullptr);
}

// called after an event completes its task. Kills any dead TCBs and loops event loop until work is
// picked up
void stop() {
    clearZombies();
    while (true) {
        event_loop(&zombieQ, nullptr);
    }
    // PANIC("Should not go here. We should be in Event Loop\n");
}

// event loop handles switching between user and kernel tasks
void event_loop(LockedQueue<TCB, SpinLock> *q, /*ISL*/ SpinLock *isl) {
    int me = getCoreID();
    // bool was = Interrupts::disable();
    auto nextThread = getNextEvent(me);
    if (nextThread == nullptr)  // try to steal work
    {
        int nextCore = (me + 1) % CORE_COUNT;
        while (nextCore != me && nextThread == nullptr) {
            nextThread = getNextEvent(nextCore);
            nextCore = (nextCore + 1) % CORE_COUNT;  // Move to the next core
        }
        if (nextThread == nullptr)  // no other threads. I can keep working/spinning
        {
            if (isl) {
                isl->unlock();
            }
            // Interrupts::restore(was);
            return;
        }
    }
    K::assert(runningThreads[me] != nullptr, "null pointer trying to run as event");

    oldThreads[me] = runningThreads[me];  // save the old thread
    // oldThreads[me]->wasDisabled = was;      // save the interrupt state prior
    // to block oldState[me] = was;               // save the interrupt state
    // prior to block
    addMeHere[me] = q;  // save the queue to add me on
    myLock[me] = isl;   // save my lock

    runningThreads[me] = nextThread;  // This is a lie, only observable by the core. its ok ?

    // cases: kernel to user, user to kernel, kernel to kernel. user to user
    if (oldThreads[me]->kernel_event && nextThread->kernel_event)  // kernel to kernel
    {
        // printf("kernel to kernel\n");
        entry();
        printf(
            "PANIC i should not go back into event loop after calling entry on "
            "a kernel event\n");                                            // should never print
    } else if (!oldThreads[me]->kernel_event && !nextThread->kernel_event)  // user to user
    {
        printf("user to user\n");
        cpu_switch_to(&((UserTCB *)oldThreads[me])->context, &((UserTCB *)nextThread)->context);
    } else if (!oldThreads[me]->kernel_event && nextThread->kernel_event)  // user to kernel
    {
        printf("user to kernel\n");
        save_context(&((UserTCB *)oldThreads[me])->context,
                     nullptr);                   // save the state of the user
        coreContext[me]->pc = (uint64_t)&entry;  // I want to go to entry to start running event
                                                 // after context switch
        load_context(nullptr, coreContext[me]);  // switch to kernel task
        // printf("PANIC post user to kernel\n");  // this should never print
    } else  // kernel to user
    {
        printf("kernel to user\n");
        cpu_switch_to(coreContext[me], &((UserTCB *)nextThread)->context);
    }
    // ASSERT(Interrupts::isDisabled());
    restoreState();
}

void restoreState() {
    // ASSERT(Interrupts::isDisabled());
    int me = getCoreID();
    auto prev = oldThreads[me];
    // auto running = runningThreads[me];
    // was someone running before me?
    if (prev) {
        // add previous thread on to desired queue
        if (addMeHere[me] != nullptr) {
            addMeHere[me]->add(prev);
        }
        // I have successfully added my prev to desired queue,
        // I can unlock the lock if I was given one
        if (myLock[me] != nullptr) {
            myLock[me]->unlock();
            // Interrupts::restore(oldThreads[me]->wasDisabled);
        }
        // Interrupts::restore(prev->wasDisabled);
    }
    // TODO:
    // --- set state based on what kind of TCB I am ---- //

    // if (running->wasDisabled)
    // {
    //     cli(); // disable
    // }
    // else
    // {
    //     Interrupts::restore(running->wasDisabled);
    // }
}

void clearZombies()  // function to delete completed events
{
    auto killMe = zombieQ.remove();
    while (killMe) {
        //  bool was = Interrupts::disable();
        // printf("addr to delete 0x%x\n", killMe);
        delete killMe;
        //  Interrupts::restore(was);
        killMe = zombieQ.remove();
    }
}

TCB* currentTCB(int core) {
    return runningThreads[core];
}
