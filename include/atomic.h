#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#include "function.h"
#include "irq.h"
#include "queue.h"
#include "stdint.h"

#ifdef USE_MONITOR
static inline void monitor(uintptr_t addr) {
    // ARM-specific or QEMU-specific implementation of monitor
    asm volatile("monitor %0" : : "r"(addr));
}
#endif

// Called when the code is spinning in a loop
//
// use_mwait (if true) tells us that the caller recommends that
// we use mwait. They are responsible for calling monitor
// as needed before checking the condition and calling us.
//
// inline void iAmStuckInALoop(bool use_mwait) {
//     if (onHypervisor) {
//         // I'm going to assume it is QEMU
//         // QEMU's support for pause and mwait is lousy and inconsistent
//         // This gives me a place to experiment.
//         if (use_mwait) {
//             mwait();
//         } else {
//             asm volatile("pause");
//         }
//     } else {
//         if (use_mwait) {
//             mwait();
//         } else {
//             asm volatile("pause");
//         }
//     }
// }

// template <typename T>
// class AtomicPtr {
//     volatile T *ptr;
// public:
//     AtomicPtr() : ptr(nullptr) {}
//     AtomicPtr(T *x) : ptr(x) {}
//     AtomicPtr<T>& operator= (T v) {
//         __atomic_store_n(ptr,v,__ATOMIC_SEQ_CST);
//         return *this;
//     }
//     operator T () const {
//         return __atomic_load_n(ptr,__ATOMIC_SEQ_CST);
//     }
//     T fetch_add(T inc) {
//         return __atomic_fetch_add(ptr,inc,__ATOMIC_SEQ_CST);
//     }
//     T add_fetch(T inc) {
//         return __atomic_add_fetch(ptr,inc,__ATOMIC_SEQ_CST);
//     }
//     void set(T inc) {
//         return __atomic_store_n(ptr,inc,__ATOMIC_SEQ_CST);
//     }
//     T get(void) {
//         return __atomic_load_n(ptr,__ATOMIC_SEQ_CST);
//     }
//     T exchange(T v) {
//         T ret;
//         __atomic_exchange(ptr,&v,&ret,__ATOMIC_SEQ_CST);
//         return ret;
//     }
// };

template <typename T>
class Atomic {
    volatile T value;

   public:
    Atomic(T x) : value(x) {
    }
    Atomic<T> &operator=(T v) {
        __atomic_store_n(&value, v, __ATOMIC_SEQ_CST);
        return *this;
    }
    operator T() const {
        return __atomic_load_n(&value, __ATOMIC_SEQ_CST);
    }
    T fetch_add(T inc) {
        return __atomic_fetch_add(&value, inc, __ATOMIC_SEQ_CST);
    }
    T add_fetch(T inc) {
        return __atomic_add_fetch(&value, inc, __ATOMIC_SEQ_CST);
    }
    void set(T inc) {
        return __atomic_store_n(&value, inc, __ATOMIC_SEQ_CST);
    }
    T get(void) {
        return __atomic_load_n(&value, __ATOMIC_SEQ_CST);
    }
    T exchange(T v) {
        T ret;
        __atomic_exchange(&value, &v, &ret, __ATOMIC_SEQ_CST);
        return ret;
    }
    void monitor_value() {
#ifdef USE_MONITOR
        monitor((uintptr_t)&value);  // Call monitor if USE_MONITOR is defined
#endif
                                     // monitor((uintptr_t)&value);
    }
};

template <>
class Atomic<uint64_t> {
   public:
    Atomic() = delete;
    Atomic(uint64_t) = delete;
};

template <>
class Atomic<int64_t> {
   public:
    Atomic() = delete;
    Atomic(int64_t) = delete;
};

class Interrupts {
    static inline uint64_t getFlags() {
        uint64_t daif;
        asm volatile("mrs %0, daif" : "=r"(daif));
        return (uint64_t)daif;
    }

   public:
    static bool isDisabled() {
        uint64_t oldFlags = getFlags();
        // double check this is the correct bit
        return (oldFlags & (1 << 7)) != 0;  // I-bit = IRQ disable
    }

    static bool disable() {
        bool wasDisabled = isDisabled();
        if (!wasDisabled) disable_irq();
        return wasDisabled;
    }

    static void restore(bool wasDisabled) {
        if (!wasDisabled) {
            enable_irq();
        }
    }

    template <typename Work>
    static inline void protect(Work work) {
        auto was = disable();
        work();
        restore(was);
    }
};

template <typename T>
class LockGuard {
    T &it;

   public:
    inline LockGuard(T &it) : it(it) {
        it.lock();
    }
    inline ~LockGuard() {
        it.unlock();
    }
};

template <typename T>
class LockGuardP {
    T *it;

   public:
    inline LockGuardP(T *it) : it(it) {
        if (it) it->lock();
    }
    inline ~LockGuardP() {
        if (it) it->unlock();
    }
};

class NoLock {
   public:
    inline void lock() {
    }
    inline void unlock() {
    }
};

// extern void pause();

class SpinLock {
    Atomic<bool> taken;

   public:
    SpinLock() : taken(false) {
    }

    SpinLock(const SpinLock &) = delete;

    // for debugging, etc. Allows false positives
    bool isMine() {
        return taken.get();
    }

    void lock(void) {
        taken.monitor_value();
        while (taken.exchange(true)) {
            // iAmStuckInALoop(true);
            taken.monitor_value();
        }
    }

    void unlock(void) {
        taken.set(false);
    }
};

class InterruptSafeLock {
    Atomic<bool> taken;
    volatile bool was;

   public:
    Atomic<uint32_t> ref_count;
    InterruptSafeLock() : taken(false), was(false), ref_count(0) {
    }

    InterruptSafeLock(const InterruptSafeLock &) = delete;

    // for debugging, etc. Allows false positives
    bool isMine() {
        return taken.get();
    }

    void lock() {
        while (true) {
            taken.monitor_value();
            bool wasDisabled = Interrupts::disable();
            if (!taken.exchange(true)) {
                was = wasDisabled;
                return;
            }
            Interrupts::restore(wasDisabled);
        }
    }

    void unlock() {
        auto wasDisabled = was;
        taken.set(false);
        Interrupts::restore(wasDisabled);
    }
};

// A more flexible InterruptSafeLock
class ISL {
    Atomic<bool> taken;

   public:
    Atomic<uint32_t> ref_count;
    ISL() : taken(false), ref_count(0) {
    }

    ISL(const ISL &) = delete;
    ISL &operator=(const ISL &) const = delete;

    // for debugging, etc. Allows false positives
    bool isMine() {
        return taken.get();
    }

    bool lock() {
        while (true) {
            taken.monitor_value();
            bool wasDisabled = Interrupts::disable();
            if (!taken.exchange(true)) {
                return wasDisabled;
            }
            Interrupts::restore(wasDisabled);
        }
    }

    // can control if interrupts are disabled or enabled when unlocking
    void unlock(bool disable) {
        taken.set(false);
        if (disable) {
            disable_irq();
        } else {
            enable_irq();
        }
    }
};

struct SemaphoreNode {
    SemaphoreNode(Function<void()> w) : work(w) {
    }

    Function<void()> work;
    SemaphoreNode *next;
};

class Semaphore {
    SpinLock spin_lock;
    Queue<SemaphoreNode> blocked_queue;
    volatile int value;

   public:
    Semaphore(int initial_value) {
        value = initial_value;
    }

    Semaphore() {
        value = 0;
    }

    Semaphore(const Semaphore &) = delete;

    void up();

    void down(Function<void()> w);
};

class Lock {
    Semaphore sema;

   public:
    Lock() : sema(1) {
    }

    Lock(const Lock &) = delete;

    void lock(Function<void()> w) {
        sema.down(w);
    }

    void unlock() {
        sema.up();
    }
};

#endif