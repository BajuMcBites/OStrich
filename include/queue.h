#ifndef _QUEUE_H
#define _QUEUE_H

#include "atomic.h"
#include "function.h"
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

    void for_each(Function<void(T *t)> consumer) {
        if (first == nullptr) return;

        T *it = first;
        while (it != nullptr) {
            consumer(it);
            it = it->next;
        }
    }
    void remove_if(Function<bool(T *t)> predicate) {
        if (first == nullptr) return;

        T *prior = nullptr;
        T *it = first;
        while (it != nullptr) {
            bool should_remove = predicate(it);
            if (should_remove) {
                if (prior == nullptr) {
                    // removing head
                    first = first->next;
                } else if (it->next == nullptr && prior != nullptr) {
                    // removing head
                    last = prior;
                    prior->next = nullptr;
                } else {
                    // removing inbetween
                    prior->next = it->next;
                }
            }
            prior = it;
            it = it->next;
        }
    }
};

#endif