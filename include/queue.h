#ifndef _QUEUE_H
#define _QUEUE_H

#include "atomic.h"
#include "heap.h"

template <typename T>
class Queue {
    T *volatile first = nullptr;
    T *volatile last = nullptr;
    volatile unsigned int size = 0;

   public:
    Queue() : first(nullptr), last(nullptr) {
        size = 0;
    }
    Queue(const Queue &) = delete;

    void add(T *t) {
        t->next = nullptr;
        if (first == nullptr) {
            first = t;
        } else {
            last->next = t;
        }
        last = t;
        size++;
    }

    T *remove() {
        if (first == nullptr) {
            return nullptr;
        }
        auto it = first;
        first = it->next;
        if (first == nullptr) {
            last = nullptr;
        }
        size--;
        return it;
    }

    T *remove_all() {
        auto it = first;
        first = nullptr;
        last = nullptr;
        size = 0;
        return it;
    }

    unsigned int get_size() {
        return size;
    }
};

#endif