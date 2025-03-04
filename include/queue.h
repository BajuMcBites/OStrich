#ifndef _QUEUE_H
#define _QUEUE_H

#include "atomic.h"
#include "heap.h"

template <typename T>
class queue {
   private:
    class node {
       public:
        node *before;
        T val;
    };
    node *tail;
    node *head;
    int sz;

   public:
    queue() {
        tail = (node *)kmalloc(sizeof(node));
        head = tail;
        sz = 0;
    }
    ~queue() {
        while (tail != head) {
            node *copy = tail;
            tail = tail->before;
            if (copy) {
                kfree(copy);
            }
        }
        if (head) {
            kfree(head);
        }
    }
    void push(T obj) {
        node *n = (node *)kmalloc(sizeof(node));
        n->val = obj;
        head->before = n;
        head = n;
        sz++;
    }
    void pop() {
        tail = tail->before;
        sz--;
    }
    T top() {
        return tail->before->val;
    }
    int size() {
        return sz;
    }
    bool empty() {
        return sz == 0;
    }
};

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