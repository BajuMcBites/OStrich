#ifndef _QUEUE_H
#define _QUEUE_H

#include "heap.h"

template <typename T>
class queue {
   private:
    class node {
       public:
        node* before;
        T val;
    };
    node* tail;
    node* head;
    int sz;

   public:
    queue() {
        tail = (node*)malloc(sizeof(node));
        head = tail;
        sz = 0;
    }
    ~queue() {
        while (tail != head) {
            node* copy = tail;
            tail = tail->before;
            if (copy) {
                free(copy);
            }
        }
        if (head) {
            free(head);
        }
    }
    void push(T obj) {
        node* n = (node*)malloc(sizeof(node));
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

#endif