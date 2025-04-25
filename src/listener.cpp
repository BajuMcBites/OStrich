#include "listener.h"

EventHandler& get_event_handler() {
    static EventHandler event_handler = EventHandler();
    return event_handler;
}