#include "peripherals/irq.h"

#include "printf.h"
#include "timer.h"
#include "utils.h"

void enable_interrupt_controller() {
    put32(ENABLE_IRQS_1, SYSTEM_TIMER_IRQ_1);
}

extern "C" void handle_irq(void) {
    unsigned int irq = get32(IRQ_PENDING_1);
    // unsigned int irq = get32(GIC_CPU_BASE + 0x0C);
    switch (irq) {
        case (SYSTEM_TIMER_IRQ_1):
            printf("timer go brr irq=%x\n", irq);
            handle_timer_irq();
            break;
        default:
            printf("Unknown pending irq: %x\r\n", irq);
    }

    // Step 3: Acknowledge the interrupt in the GIC by writing to the End of Interrupt Register
    // (ICC_EOI)
    put32(GIC_CPU_BASE + 0x10, irq);  // Write the IRQ number to ICC_EOI (End of Interrupt)

    irq = get32(IRQ_PENDING_1);
    printf("is it gone irq: %x\r\n", irq);
}

void gic_init(void) {
    // Enable the GIC Distributor
    // volatile uint32_t *GIC_DIST_CTRL = (uint32_t *)(GIC_DIST_BASE + 0x0000);
    // *GIC_DIST_CTRL = 0x1;  // Enable the GIC Distributor

    put32(GIC_DIST_BASE, 0x1);

    // Set the GIC CPU Interface to be active
    // volatile uint32_t *GIC_CPU_CTRL = (uint32_t *)(GIC_CPU_BASE + 0x0000);
    // *GIC_CPU_CTRL = 0x1;  // Enable the CPU Interface

    put32(GIC_CPU_BASE, 0x1);

    // Step 4: Enable specific interrupts in the distributor and target the correct CPU (this may be
    // done per interrupt).
    // Example for enabling timer interrupts:
    put32(GIC_DIST_BASE + 0x0100,
          SYSTEM_TIMER_IRQ_1);  // Enable SYSTEM_TIMER_IRQ_1 in the distributor

    // Step 5: Enable the GIC CPU interface to accept interrupts
    // You might want to configure the interrupt priorities and targets per-core.
    // For now, we assume defaults for simplicity.

    // Enable the timer interrupt for this core
    put32(GIC_CPU_BASE + 0x100, 0x3);  // Enable the interrupt for the CPU interface
}