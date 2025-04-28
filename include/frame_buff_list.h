#ifndef FRAMEBUFFER_LINKED_LIST_H
#define FRAMEBUFFER_LINKED_LIST_H

#include "framebuffer.h"
#include "libk.h"
#include "stdint.h"

class FrameBufferLinkedList {
   private:
    Framebuffer* head;

   public:
    FrameBufferLinkedList() : head(nullptr) {
    }

    void insert(Framebuffer* fb) {
        if (!fb) return;

        if (!head) {
            // Initialize the list with the first framebuffer
            head = fb;
            fb->next = fb;
            fb->prev = fb;
        } else {
            Framebuffer* tail = head->prev;
            tail->next = fb;
            fb->prev = tail;
            fb->next = head;
            head->prev = fb;

            // Make the new framebuffer the new head
            head = fb;
        }
    }

    void clear() {
        if (!head) return;

        Framebuffer* current = head;
        do {
            Framebuffer* temp = current;
            current = current->next;
            delete temp;
        } while (current != head);
        head = nullptr;
    }

    Framebuffer* getHead() const {
        K::assert(!isEmpty(), "cannot get from empty list");
        return head;
    }

    void moveHeadForward() {
        if (head) head = head->next;
    }

    void moveHeadBack() {
        if (head) head = head->prev;
    }

    bool isEmpty() const {
        return head == nullptr;
    }

    size_t size() const {
        if (!head) return 0;
        size_t count = 0;
        Framebuffer* current = head;
        do {
            count++;
            current = current->next;
        } while (current != head);
        return count;
    }
};

#endif  // FRAMEBUFFER_LINKED_LIST_H
