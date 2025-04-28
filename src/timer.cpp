#include "peripherals/timer.h"

#include "irq.h"
#include "peripherals/arm_devices.h"
#include "printf.h"
#include "utils.h"

const unsigned int interval = 200000;
unsigned int curVal = 0;

void timer_init(void) {
    curVal = get32(TIMER_CLO);
    curVal += interval;
    put32(TIMER_C1, curVal);
}

void local_timer_init() {
    QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE0_IRQ;  // Route local timer IRQ to Core0
    QA7->TimerControlStatus.ReloadValue = 10000000;       // Timer period set
    QA7->TimerControlStatus.TimerEnable = 1;              // Timer enabled
    QA7->TimerControlStatus.IntEnable = 1;                // Timer IRQ enabled
    QA7->TimerClearReload.IntClear = 1;                   // Clear interrupt
    QA7->TimerClearReload.Reload = 1;                     // Reload now

    // We are in NS EL1 so enable IRQ to core0 that level
    // Make sure FIQ is zero, if set irq is ignored
    QA7->Core0TimerIntControl.nCNTPNSIRQ_IRQ = 1;
    QA7->Core0TimerIntControl.nCNTPNSIRQ_FIQ = 0;
    QA7->Core1TimerIntControl.nCNTPNSIRQ_IRQ = 1;
    QA7->Core1TimerIntControl.nCNTPNSIRQ_FIQ = 0;
    QA7->Core2TimerIntControl.nCNTPNSIRQ_IRQ = 1;
    QA7->Core2TimerIntControl.nCNTPNSIRQ_FIQ = 0;
    QA7->Core3TimerIntControl.nCNTPNSIRQ_IRQ = 1;
    QA7->Core3TimerIntControl.nCNTPNSIRQ_FIQ = 0;

    // set up mailbox
    QA7->Core0MailboxIntControl.Mailbox0_IRQ = 1;
    QA7->Core0MailboxIntControl.Mailbox0_FIQ = 0;

    QA7->Core1MailboxIntControl.Mailbox0_IRQ = 1;
    QA7->Core1MailboxIntControl.Mailbox0_FIQ = 0;

    QA7->Core2MailboxIntControl.Mailbox0_IRQ = 1;
    QA7->Core2MailboxIntControl.Mailbox0_FIQ = 0;

    QA7->Core3MailboxIntControl.Mailbox0_IRQ = 1;
    QA7->Core3MailboxIntControl.Mailbox0_FIQ = 0;
}

extern "C" void handle_timer_irq(void) {
    curVal += interval;
    put32(TIMER_C1, curVal);
    put32(TIMER_CS, TIMER_CS_M1);
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

/**
 * gets time since system start up in microseconds
 */
uint64_t get_systime() {
    register unsigned long f, t, r;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(f));
    asm volatile("mrs %0, cntpct_el0" : "=r"(t));
    return (t * 1000000) / f;
}