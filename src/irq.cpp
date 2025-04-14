#include "peripherals/irq.h"

#include "peripherals/arm_devices.h"
#include "peripherals/timer.h"
#include "printf.h"
#include "timer.h"
#include "utils.h"
#include "atomic.h"

/*
https://developer.arm.com/documentation/ddi0500/j/Generic-Interrupt-Controller-CPU-Interface/GIC-programmers-model/CPU-interface-register-summary

https://developer.arm.com/documentation/102379/0104/The-processor-timers/Timer-registers

https://jsandler18.github.io/extra/interrupts.html

https://developer.arm.com/documentation/102909/0100/The-Generic-Interrupt-Controller

*/
// #define GICD_BASE 0x3F001000
// #define GICC_BASE 0x3F002000

void enable_interrupt_controller() {
    put32(ENABLE_IRQS_1, SYSTEM_TIMER_IRQ_1);
    // uint32_t *GICD_ISENABLER = (uint32_t *)(GICD_BASE + 0x100);
//     *GICD_ISENABLER |= (1 << 29); // Enable interrupt ID 29 (Local Timer)

//     // Configure CPU Interface
//     uint32_t *GICC_CTLR = (uint32_t *)(GICC_BASE + 0x0);
//     *GICC_CTLR |= 1; // Enable CPU interface
}

extern "C" void handle_irq(void) {
    printf("core %d is irq handle?\n", getCoreID());

    uint8_t current_core = getCoreID();
    
    // timer is triggered
    if (QA7->Core0IRQSource.Timer_Int && current_core == curr_interrupt) {
        

        int next_core = (current_core + 1) % 4;
        curr_interrupt = next_core;
        QA7->TimerRouting.Routing = static_cast<local_timer_route_num_t>(next_core);
        
        // clear interrupt and reload timer at the same time
        QA7->TimerClearReload.Raw32 = (1 << 31) | (1 << 30); 
    }

    // unsigned int irq = get32(IRQ_PENDING_1);
    // // unsigned int irq = get32(GIC_CPU_BASE + 0x0C);
    // // unsigned int irq = get32(GIC_CPU_BASE + 0x0C);  // GICC_IAR
    // switch (irq) {
    //     case (SYSTEM_TIMER_IRQ_1):
    //         printf("timer go brr  on core %d irq=%x\n", getCoreID(), irq);
    //         // printf("timer go brr irq=%x\n", irq);
    //         handle_timer_irq();
    //         // volatile uint32_t *LOCAL_TIMER_CLR = (uint32_t *)(0x4000003C + VA_START);
    //         // *LOCAL_TIMER_CLR = 1;  // Clear the timer interrupt
    //         break;
    //     default:
    //         printf("Unknown pending irq: %x\r\n", irq);
    // }
    // QA7->TimerClearReload.IntClear = 1;  // Clear interrupt
    // QA7->TimerClearReload.Reload = 1;    // Reload now

    // Step 3: Acknowledge the interrupt in the GIC by writing to the End of Interrupt Register
    // (ICC_EOI)
    // put32(GIC_CPU_BASE + 0x10, irq);  // Write the IRQ number to ICC_EOI (End of Interrupt)

    // irq = get32(IRQ_PENDING_1);
    // printf("is it gone irq: %x\r\n", irq);
}

// void gic_init(void) {
//     // Enable interrupt in GIC Distributor (GICD_ISENABLER0)
//     return;
// }
