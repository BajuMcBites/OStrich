#include "peripherals/timer.h"
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