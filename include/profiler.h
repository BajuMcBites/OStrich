#ifndef _PROFILER_H
#define _PROFILER_H

#include <stdint.h>

typedef struct {
    uint64_t func;
    uint64_t caller;
    uint64_t time;
    uint64_t flags;
} __attribute__((aligned(8))) entry_t;

entry_t *get_buffer();

extern uint32_t enqueue_idx;
extern uint32_t dequeue_idx;
extern bool initialized;
extern int depth;
extern int entries;

__attribute__((no_instrument_function)) void profiler_flush_loop();

extern "C" {
__attribute__((no_instrument_function)) void __cyg_profile_func_enter(void *, void *);
__attribute__((no_instrument_function)) void __cyg_profile_func_exit(void *, void *);
};

#endif