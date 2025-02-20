#ifndef _queue_h_
#define _queue_h_

#include "atomic.h"

template <typename T, typename LockType>
class Queue
{
    T *volatile first = nullptr;
    T *volatile last = nullptr;
    LockType lock;

public:
    Queue() : first(nullptr), last(nullptr), lock() {}
    Queue(const Queue &) = delete;

    void add(T *t)
    {
        LockGuard g{lock};
        t->next = nullptr;
        if (first == nullptr)
        {
            first = t;
        }
        else
        {
            last->next = t;
        }
        last = t;
    }

    T *remove()
    {
        LockGuard g{lock};
        if (first == nullptr)
        {
            return nullptr;
        }
        auto it = first;
        first = it->next;
        if (first == nullptr)
        {
            last = nullptr;
        }
        return it;
    }

    T *remove_all()
    {
        LockGuard g{lock};
        auto it = first;
        first = nullptr;
        last = nullptr;
        return it;
    }
};

#endif
