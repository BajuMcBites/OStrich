#ifndef _QUEUE_H
#define _QUEUE_H

// SWITCH TO MALLOC WHEN ITS DONE

template <typename T> class queue {
    private: 
        class node {
            *node before;
            *T val; 
        };
        node* tail;
        node* head;
        int size;
    public:
        queue() {
            node new_tail = {NULL, NULL};
            tail = &node;
            head = tail;
            size = 0;
        }
        void push (T obj) {
            node n = {NULL, obj};
            head->before = n;
            head = n;
            size++;
        }
        void pop() {
            tail = tail->before;
            size--;
        }
        T top() {
            return tail->before->val;
        }
        int size() {
            return size;
        }
        bool empty() {
            return size == 0;
        }

} 


#endif