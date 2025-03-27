#include "peripherals/irq.h"

#include "peripherals/timer.h"
#include "printf.h"
#include "timer.h"
#include "utils.h"

/*
https://developer.arm.com/documentation/ddi0500/j/Generic-Interrupt-Controller-CPU-Interface/GIC-programmers-model/CPU-interface-register-summary


*/

void enable_interrupt_controller() {
    put32(ENABLE_IRQS_1, SYSTEM_TIMER_IRQ_1);
}

extern "C" void handle_irq(void) {
    unsigned int irq = get32(IRQ_PENDING_1);
    // unsigned int irq = get32(GIC_CPU_BASE + 0x0C);
    // unsigned int irq = get32(GIC_CPU_BASE + 0x0C);  // GICC_IAR
    switch (irq) {
        case (SYSTEM_TIMER_IRQ_1):
            printf("timer go brr  on core %d irq=%x\n", getCoreID(), irq);
            // printf("timer go brr irq=%x\n", irq);
            handle_timer_irq();
            break;
        default:
            printf("Unknown pending irq: %x\r\n", irq);
    }

    // Step 3: Acknowledge the interrupt in the GIC by writing to the End of Interrupt Register
    // (ICC_EOI)
    // put32(GIC_CPU_BASE + 0x10, irq);  // Write the IRQ number to ICC_EOI (End of Interrupt)

    // irq = get32(IRQ_PENDING_1);
    // printf("is it gone irq: %x\r\n", irq);
}

void gic_init(void) {
    // Enable interrupt in GIC Distributor (GICD_ISENABLER0)
    return;
}
