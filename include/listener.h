#ifndef _LISTENER_H
#define _LISTENER_H

#include "atomic.h"
#include "event.h"
#include "hash.h"
#include "heap.h"
#include "queue.h"

#define KEYBOARD_EVENT 1
#define MOUSE_EVENT 2

#define ARP_RESOLVED_EVENT 0x100000000
#define DNS_RESOLVED_EVENT 0x200000000
#define ICMP_PING_EVENT 0x400000000
#define TEMP_EVENT_MASK 0xFFFFFFFF00000000

struct IListener {
    uint32_t _id;
    struct IListener *next;
};

template <typename... Args>
struct Listener : public IListener {
    Function<void(Args...)> handler;
    Function<void()> cleanup;

    Listener() {
    }
    Listener(Function<void(Args...)> &&handler) : handler(std::move(handler)) {
        this->cleanup = [&]() { delete this; };
    }

    Listener(Function<void(Args...)> &&handler, Function<void()> &&clean_up)
        : handler(std::move(handler)), cleanup(std::move(clean_up)) {
    }

    ~Listener() {
    }
};

class EventHandler {
   public:
    EventHandler() {
        this->map = new HashMap<uint64_t, Queue<IListener> *>(uint64_t_hash, uint64_t_equals, 10);
    }

    ~EventHandler() {
        this->map->for_each([](Queue<IListener> *queue) { delete queue; });
        delete this->map;
    }

    template <typename... Args>
    void register_listener(int event_id, Listener<Args...> *listener) {
        static int listener_id = 0;
        if (this->map->get(event_id) == nullptr) {
            Queue<IListener> *q = new Queue<IListener>();
            this->map->put(event_id, q);
        }
        listener->_id = listener_id++;
        this->map->get(event_id)->add(reinterpret_cast<IListener *>(listener));
    }

    void unregister_listener(int event_id, uint32_t listener_id) {
        this->map->get(event_id)->remove_if(
            [&listener_id](IListener *listener) -> bool { return listener->_id == listener_id; });
    }

    template <typename... Args>
    void handle_event(int event_id, Args &...args) {
        this->handle_event(event_id, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void handle_event(int event_id, Args &&...args) {
        if (auto q = this->map->get(event_id)) {
            q->for_each([&args...](IListener *listener) {
                reinterpret_cast<Listener<Args...> *>(listener)->handler(args...);
            });

            if (event_id & TEMP_EVENT_MASK) {
                q->for_each([](IListener *listener) {
                    reinterpret_cast<Listener<Args...> *>(listener)->cleanup();
                });
                this->map->remove(event_id);
            }
        }
    }

   protected:
    HashMap<uint64_t, Queue<IListener> *> *map;
};

EventHandler *get_event_handler();

#endif