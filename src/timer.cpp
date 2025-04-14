#include "peripherals/timer.h"

#include "irq.h"
#include "printf.h"
#include "sched.h"
#include "utils.h"
#include "peripherals/arm_devices.h"

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

void init_core_timer(void) {
    uint8_t core_id = getCoreID();
    volatile generic_timer_int_ctrl_reg_t *ctrl_reg;

    switch (core_id) {
        case 0: 
            ctrl_reg = &QA7->Core0TimerIntControl; 
            break;
        case 1: 
            ctrl_reg = &QA7->Core1TimerIntControl; 
            break;
        case 2: 
            ctrl_reg = &QA7->Core2TimerIntControl; 
            break;
        case 3: 
            ctrl_reg = &QA7->Core3TimerIntControl; 
            break;
    }

    ctrl_reg->nCNTPNSIRQ_IRQ = 1;  // We are in NS EL1 so enable IRQ to core0 that level
    ctrl_reg->nCNTPNSIRQ_FIQ = 0;  // disable FIQ override
}

extern "C" void handle_timer_irq(void) {
    curVal += interval;
    put32(TIMER_C1, curVal);
    put32(TIMER_CS, TIMER_CS_M1);
    timer_tick();
}