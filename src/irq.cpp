#include "utils.h"
#include "printf.h"
#include "timer.h"
#include "peripherals/irq.h"

void enable_interrupt_controller()
{
	put32(ENABLE_IRQS_1, SYSTEM_TIMER_IRQ_1);
}

extern "C" void handle_irq(void)
{
	unsigned int irq = get32(IRQ_PENDING_1);
	switch (irq) {
		case (SYSTEM_TIMER_IRQ_1):
			handle_timer_irq();
			break;
		default:
			printf("Unknown pending irq: %x\r\n", irq);
	}
}