#include "peripherals/timer.h"

#include "irq.h"
#include "printf.h"
#include "sched.h"
#include "utils.h"

const unsigned int interval = 200000;
unsigned int curVal = 0;

void timer_init(void) {
    curVal = get32(TIMER_CLO);
    curVal += interval;
    put32(TIMER_C1, curVal);
}

void core_timer_init(void) {
    volatile uint32_t *LOCAL_TIMER_CTRL = (uint32_t *)(0x40000034 + VA_START);
    volatile uint32_t *LOCAL_TIMER_RELOAD = (uint32_t *)(0x40000038 + VA_START);
    // volatile uint32_t *LOCAL_TIMER_CTRL = (uint32_t *)0x40000034;
    // volatile uint32_t *LOCAL_TIMER_RELOAD = (uint32_t *)0x40000038;

    *LOCAL_TIMER_RELOAD = 1000000;              // Set timer interval
    *LOCAL_TIMER_CTRL = (1 << 29) | (1 << 28);  // Enable timer + IRQ
    // rearm_timer();
}

extern "C" void handle_timer_irq(void) {
    curVal += interval;
    put32(TIMER_C1, curVal);
    put32(TIMER_CS, TIMER_CS_M1);
    timer_tick();
}

/**
 * Wait N microsec (ARM CPU only)
 */
void wait_msec(unsigned int n) {
    register unsigned long f, t, r;
    // get the current counter frequency
    asm volatile("mrs %0, cntfrq_el0" : "=r"(f));
    // read the current counter
    asm volatile("mrs %0, cntpct_el0" : "=r"(t));
    // calculate required count increase
    unsigned long i = ((f / 1000) * n) / 1000;
    // loop while counter increase is less than i
    do {
        asm volatile("mrs %0, cntpct_el0" : "=r"(r));
    } while (r - t < i);
}