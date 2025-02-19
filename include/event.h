#ifndef EVENT_H
#define EVENT_H

#include "heap.h"
#include "queue.h"

struct event {
    explicit inline event() {}
    virtual void run() = 0;
    virtual ~event() {}
};
  
template <typename work>
struct event_work : public event {
    work const w;
  
    explicit inline event_work(work const w) : event(), w(w) {}
    virtual void run() override { w(); }
};
  
template <typename work, typename T>
struct event_work_value : public event_work<work> {
    T* value;
  
    explicit inline event_work_value(work const w, T* value) : event_work<work>(w), value(value) {}
    virtual void run() override {
        w(value);
    }
};
  
#endif // EVENT_H