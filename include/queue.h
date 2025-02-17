#ifndef _QUEUE_H
#define _QUEUE_H

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
            tail = malloc(sizeof(node));
            head = tail;
            size = 0;
        }
        ~queue() {
            while(tail != head) {
                node *copy = tail;
                tail = tail->before;
                free(copy);
            }
            free(head);
        }
        void push (T obj) {
            node* n = malloc(sizeof(node));
            n->val = obj;
            head->before = n;
            head = n;
            size++;
        }
        void pop() {
            node *copy = tail;
            tail = tail->before;
            free(copy);
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