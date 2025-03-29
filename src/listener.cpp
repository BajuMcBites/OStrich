#include "listener.h"

EventHandler *event_handler;

void event_listener_init() {
    static bool initialized = false;
    if(initialized) return;
    event_handler = new EventHandler{};
    initialized = true;
}