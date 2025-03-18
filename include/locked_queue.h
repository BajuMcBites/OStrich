#ifndef _LOCKED_QUEUE_H
#define _LOCKED_QUEUE_H

#include "atomic.h"
#include "heap.h"


template <typename T, typename LockType>
class LockedQueue {
    T *volatile first = nullptr;
    T *volatile last = nullptr;
    LockType lock;

   public:
    LockedQueue() : first(nullptr), last(nullptr), lock() {
    }
    LockedQueue(const LockedQueue &) = delete;

    void add(T *t) {
        LockGuard g{lock};
        t->next = nullptr;
        if (first == nullptr) {
            first = t;
        } else {
            last->next = t;
        }
        last = t;
    }

    T *remove() {
        LockGuard g{lock};
        if (first == nullptr) {
            return nullptr;
        }
        auto it = first;
        first = it->next;
        if (first == nullptr) {
            last = nullptr;
        }
        return it;
    }

    T *remove_all() {
        LockGuard g{lock};
        auto it = first;
        first = nullptr;
        last = nullptr;
        return it;
    }
};

#endif

