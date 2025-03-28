#ifndef _LISTENER_H
#define _LISTENER_H

#include "atomic.h"
#include "hash.h"
#include "heap.h"
#include "printf.h"
#include "queue.h"

#define KEYBOARD_EVENT 1
#define MOUSE_EVENT 2

template <typename... args>
struct Listener {
    void (*handler)(args... a);
    uint32_t _id;
    struct Listener<args...> *next;
};

class EventHandler {
   public:
    EventHandler() {
        this->map = new HashMap<Queue<Listener<void *>>*>(10);
    }

    ~EventHandler() {
        delete &this->map;
    }

    template <typename... Args>
    void register_listener(int event_id, Listener<Args...> *listener) {
        static int listener_id = 0;
        if (this->map->get(event_id) == nullptr) {
            Queue<Listener<void *>> *q =
                (Queue<Listener<void *>> *)kmalloc(sizeof(Queue<Listener<void>>));
            this->map->put(event_id, q);
        }
        listener->_id = listener_id++;
        this->map->get(event_id)->add(reinterpret_cast<Listener<void *> *>(listener));
    }


    void unregister_listener(int event_id, int listener_id){
        this->map->get(event_id)->remove_if([&listener_id](Listener<void *> *listener) -> bool {
            return listener->_id == listener_id;
        });
    }

    template <typename... Args>
    void handle_event(int event_id, Args &&...args) {
        if (this->map->get(event_id) == nullptr) return;
        Function<void(Listener<void *>*)> consumer = [&args...](Listener<void *> *listener) {
            if (listener->handler != nullptr) {
                reinterpret_cast<Listener<Args...>*>(listener)->handler(args...);
            }
        };
        this->map->get(event_id)->for_each(consumer);
    }

   protected:
    HashMap<Queue<Listener<void *>>*> *map;
};

#endif