#ifndef _SCHED_H
#define _SCHED_H

enum event_type {
    OPEN,
    CLOSE,
};

class event {
    event_type type;
    uint32_t pid;
};

#endif