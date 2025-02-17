#ifndef _SCHED_H
#define _SCHED_H

enum event_type {
    NEW,
    READY,
    BLOCK,
    RUN,
    TERMINATE,
};

class event {
    event_type type;
    uint32_t pid;
};

void process_event( void );

#endif