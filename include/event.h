// #pragma once
// #ifndef _EVENT_H
// #define _EVENT_H

// #include "function.h"
// #include "heap.h"
// #include "queue.h"

// struct event {
//     explicit inline event() {
//     }
//     virtual void run() = 0;
//     virtual ~event() {
//     }
// };

// struct event_work : public event {
//     Function<void()> w;
//     template <typename work>
//     explicit inline event_work(work w) : event(), w(w) {
//     }
//     virtual void run() override {
//         w();
//     }
// };

// template <typename T>
// struct event_work_value : public event
// {
//     Function<void(T)> w;
//     T value;

//     template <typename work>
//     explicit inline event_work_value(work w, T value) : event(), w(w), value(value) {
//     }
//     virtual void run() override {
//         w(value);
//     }
// };

// #endif  // _EVENT_H