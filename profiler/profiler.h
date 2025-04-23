#ifndef _PROFILER_H
#define _PROFILER_H

#include "stdint.h"

#define TRACE_SIZE 32
#define ITEMS_PER_READ 64
#define TRACE_CORE_COUNT 4

#define TRACE_ENTRY false
#define TRACE_EXIT true

struct function_call {
    uint64_t function;
    uint64_t start_time;

    function_call() : function(0), start_time(0) {
    }
    function_call(uint64_t function, uint64_t start_time)
        : function(function), start_time(start_time) {
    }
};

struct function_trace {
    uint64_t num_executions;
    uint64_t total_time;

    function_trace(uint64_t num_exec, uint64_t total_time)
        : num_executions(num_exec), total_time(total_time) {
    }
};

typedef struct {
    uint64_t function;
    uint64_t caller;
    uint64_t time;
    uint64_t flags;
} entry_t;

#endif