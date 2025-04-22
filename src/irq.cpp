#include "peripherals/irq.h"

#include "dwc.h"
#include "event.h"
#include "irq.h"
#include "peripherals/arm_devices.h"
#include "peripherals/timer.h"
#include "printf.h"
#include "sys.h"
#include "timer.h"
#include "utils.h"

/*
https://developer.arm.com/documentation/ddi0500/j/Generic-Interrupt-Controller-CPU-Interface/GIC-programmers-model/CPU-interface-register-summary

https://developer.arm.com/documentation/102379/0104/The-processor-timers/Timer-registers

https://jsandler18.github.io/extra/interrupts.html

https://developer.arm.com/documentation/102909/0100/The-Generic-Interrupt-Controller

*/

void enable_interrupt_controller() {
    put32(ENABLE_IRQS_1, SYSTEM_TIMER_IRQ_1);
}

/*
    core_int_source_reg_t Core0IRQSource;               // 0x60
    core_int_source_reg_t Core1IRQSource;               // 0x64
    core_int_source_reg_t Core2IRQSource;               // 0x68
    core_int_source_reg_t Core3IRQSource;               // 0x6C

*/

Atomic<int> wait{0};

extern "C" void handle_irq(KernelEntryFrame* frame) {
    auto me = getCoreID();
    auto event = get_running_task(me);
    event->irq_was_disabled = Interrupts::disable();  // disable returns the irq status before irq

    core_int_source_reg_t irq_source;
    if (me == 0) {
        irq_source.Raw32 = QA7->Core0IRQSource.Raw32;
        // QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE1_IRQ;
        // MAIL->mailboxSet.Core1Mailbox0Set = 2;
        // MAIL->mailboxSet.Core2Mailbox0Set = 2;
        // MAIL->mailboxSet.Core3Mailbox0Set = 2;
        // put32(CORE1_MBOX0_SET, 1);
    } else if (me == 1) {
        irq_source.Raw32 = QA7->Core1IRQSource.Raw32;
        // QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE2_IRQ;
        // //  disable_irq();
        // MAIL->mailboxSet.Core0Mailbox0Set = 2;
        // MAIL->mailboxSet.Core2Mailbox0Set = 2;
        // MAIL->mailboxSet.Core3Mailbox0Set = 2;

    } else if (me == 2) {
        irq_source.Raw32 = QA7->Core2IRQSource.Raw32;
        // QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE3_IRQ;
        // MAIL->mailboxSet.Core0Mailbox0Set = 2;
        // MAIL->mailboxSet.Core1Mailbox0Set = 2;
        // MAIL->mailboxSet.Core3Mailbox0Set = 2;
    } else {
        irq_source.Raw32 = QA7->Core3IRQSource.Raw32;
        put32(CORE0_MBOX0_SET, 1);
        put32(CORE1_MBOX0_SET, 1);
        put32(CORE2_MBOX0_SET, 1);
        // QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE0_IRQ;
        // MAIL->mailboxSet.Core0Mailbox0Set = 2;
        // MAIL->mailboxSet.Core1Mailbox0Set = 2;
        // MAIL->mailboxSet.Core2Mailbox0Set = 2;
    }

    uint32_t interrupt_source = 0;
    if (me == 0)
        interrupt_source = QA7->Core0IRQSource.Raw32;
    else if (me == 1)
        interrupt_source = QA7->Core1IRQSource.Raw32;
    else if (me == 2)
        interrupt_source = QA7->Core2IRQSource.Raw32;
    else
        interrupt_source = QA7->Core3IRQSource.Raw32;

    // core_int_source_reg_t irq = (core_int_source_reg_t) irq_source;
    if (irq_source.Timer_Int) {
        // handle timer irq
        printf("Core %d in timer irq\n", getCoreID());
        yield(frame);

        // (uintptr_t)(0x40000024 + VA_START)
    } else {
        if (me == 0) {
            printf("UNKNOWN irq: 0x%x,  Core %d in irq\n", interrupt_source, me);
            printf("read %d\n", get32(CORE0_MBOX0_RDCLR));
            printf("read %d\n", MAIL->mailboxClear.Core0Mailbox0Clr);
            MAIL->mailboxClear.Core0Mailbox0Clr = 0;
            put32(CORE0_MBOX0_RDCLR, 0);
            put32(CORE0_MBOX0_SET, 0);
        } else if (me == 1) {
            printf("UNKNOWN irq: 0x%x,  Core %d in irq\n", interrupt_source, me);
            printf("read %d\n", get32(CORE1_MBOX0_RDCLR));
            printf("read %d\n", MAIL->mailboxClear.Core1Mailbox0Clr);
            MAIL->mailboxClear.Core1Mailbox0Clr = 0;

            put32(CORE1_MBOX0_RDCLR, 0);
            put32(CORE1_MBOX0_SET, 0);

        } else if (me == 2) {
            printf("UNKNOWN irq: 0x%x,  Core %d in irq\n", interrupt_source, me);
            printf("read %d\n", get32(CORE2_MBOX0_RDCLR));
            printf("read %d\n", MAIL->mailboxClear.Core2Mailbox0Clr);
            MAIL->mailboxClear.Core2Mailbox0Clr = 0;

            put32(CORE2_MBOX0_SET, 0);
            put32(CORE2_MBOX0_RDCLR, 0);
        } else {
            printf("UNKNOWN irq: 0x%x,  Core %d in irq\n", interrupt_source, me);
            printf("read %d\n", get32(CORE3_MBOX0_RDCLR));
            printf("read %d\n", MAIL->mailboxClear.Core3Mailbox0Clr);
            MAIL->mailboxClear.Core3Mailbox0Clr = 0;
            put32(CORE3_MBOX0_RDCLR, 0);
            put32(CORE3_MBOX0_SET, 0);
        }
    }
    Interrupts::restore(event->irq_was_disabled);
}
