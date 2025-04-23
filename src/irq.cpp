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
        QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE1_IRQ;
    } else if (me == 1) {
        irq_source.Raw32 = QA7->Core1IRQSource.Raw32;
        QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE2_IRQ;

    } else if (me == 2) {
        irq_source.Raw32 = QA7->Core2IRQSource.Raw32;
        QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE3_IRQ;
    } else {
        irq_source.Raw32 = QA7->Core3IRQSource.Raw32;
        QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE0_IRQ;
    }

    // uint32_t interrupt_source = 0;
    // if (me == 0)
    //     interrupt_source = QA7->Core0IRQSource.Raw32;
    // else if (me == 1)
    //     interrupt_source = QA7->Core1IRQSource.Raw32;
    // else if (me == 2)
    //     interrupt_source = QA7->Core2IRQSource.Raw32;
    // else
    //     interrupt_source = QA7->Core3IRQSource.Raw32;

    // core_int_source_reg_t irq = (core_int_source_reg_t) irq_source;
    if (irq_source.Timer_Int) {
        // handle timer irq
        printf("Core %d in timer irq\n", getCoreID());
        yield(frame);

        // (uintptr_t)(0x40000024 + VA_START)
    } else {
        printf("UNKNOWN irq: 0x%x,  Core %d in irq\n", irq_source.Raw32, me);
    }
    Interrupts::restore(event->irq_was_disabled);
}
