#include "profiler.h"

#pragma GCC push_options
#pragma GCC no_instrument_function
#include "printf.h"
#include "utils.h"
#pragma GCC pop_options

#define UART1_BASE 0x3F215000
#define UART1_IO_REG (UART1_BASE + 0x40)
#define UART1_LSR_REG (UART1_BASE + 0x54)
#define BUFFER_SIZE 40000

__attribute__((no_instrument_function)) void uart1_putc(char c) {
    while ((*((volatile uint32_t *)(UART1_LSR_REG)) & (1 << 5)) == 0) {
    }
    *((uint32_t *)UART1_IO_REG) = (uint32_t)c;
}

__attribute__((no_instrument_function)) void uart1_put64(uint64_t val) {
    uint64_t n;
    int c;
    for (c = 0; c < 64; c += 8) {
        n = (val >> c) & 0xFF;
        uart1_putc(n);
    }
}

__attribute__((no_instrument_function)) entry_t *get_buffer() {
    static uint8_t buffer[BUFFER_SIZE * 8 * 4];
    return (entry_t *)buffer;
}

int entries = 0;
int depth = -1;
uint32_t enqueue_idx = 0;
uint32_t dequeue_idx = 0;

void profiler_flush_loop() {
    uint32_t start;
    uint32_t stop;

    uint32_t count = 0;
    uint32_t functions_logged = 0;
    while (1) {
        start = dequeue_idx;
        stop = enqueue_idx;
        if (start > stop) stop += (BUFFER_SIZE);

        if (stop - start >= 128) {
            functions_logged += stop - start;
            for (; start < stop; start++) {
                uart1_put64(get_buffer()[start % BUFFER_SIZE].func);
                uart1_put64(get_buffer()[start % BUFFER_SIZE].caller);
                uart1_put64(get_buffer()[start % BUFFER_SIZE].time);
                uart1_put64(get_buffer()[start % BUFFER_SIZE].flags);
            }
            dequeue_idx = stop % BUFFER_SIZE;
            count++;
        }

        if (functions_logged > 10) {
            break;
        }
    }
}

extern "C" {

void __cyg_profile_func_enter(void *func, void *caller) {
    uint64_t time;

    asm volatile("mrs %0, cntpct_el0" : "=r"(time));

    get_buffer()[enqueue_idx].func = (uint64_t)func;
    get_buffer()[enqueue_idx].caller = (uint64_t)caller;
    get_buffer()[enqueue_idx].time = time;
    get_buffer()[enqueue_idx].flags = (uint64_t)1 | (getCoreID() << 8);

    enqueue_idx = (enqueue_idx + 1) % (BUFFER_SIZE);
    entries++;
}

void __cyg_profile_func_exit(void *func, void *caller) {
    uint64_t time;

    asm volatile("mrs %0, cntpct_el0" : "=r"(time));

    get_buffer()[enqueue_idx].func = (uint64_t)func;
    get_buffer()[enqueue_idx].caller = (uint64_t)caller;
    get_buffer()[enqueue_idx].time = time;
    get_buffer()[enqueue_idx].flags = (uint64_t)0 | (getCoreID() << 8);
    enqueue_idx = (enqueue_idx + 1) % (BUFFER_SIZE);
    entries++;
}
};