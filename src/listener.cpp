#include "listener.h"

EventHandler *get_event_handler() {
    static EventHandler *event_handler = nullptr;
    if(event_handler == nullptr) event_handler = new EventHandler();
    return event_handler;
}