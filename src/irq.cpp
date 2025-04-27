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

extern "C" void handle_irq(KernelEntryFrame* frame) {
    auto me = getCoreID();
    auto event = get_running_task(me);
    K::assert(event != nullptr, "nullptr in irq\n");
    event->irq_was_disabled = Interrupts::disable();  // disable returns the irq status before
    core_int_source_reg_t irq_source;
    if (me == 0) {
        irq_source.Raw32 = QA7->Core0IRQSource.Raw32;
    } else if (me == 1) {
        irq_source.Raw32 = QA7->Core1IRQSource.Raw32;
    } else if (me == 2) {
        irq_source.Raw32 = QA7->Core2IRQSource.Raw32;
    } else {
        irq_source.Raw32 = QA7->Core3IRQSource.Raw32;
    }

    if (irq_source.Timer_Int) {
        //  printf("Core %d in timer irq\n", getCoreID());
        yield(frame);
        QA7->TimerClearReload.IntClear = 1;  // Clear interrupt
        if (me == 0) {
            QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE1_IRQ;
        } else if (me == 1) {
            QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE2_IRQ;
        } else if (me == 2) {
            QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE3_IRQ;
        } else {
            QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE0_IRQ;
        }
        QA7->TimerClearReload.Reload = 1;  // Reload now
    } else {
        printf("UNKNOWN irq: 0x%x,  Core %d in irq\n", irq_source.Raw32, me);
    }
    Interrupts::restore(event->irq_was_disabled);
}
