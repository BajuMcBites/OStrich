#ifndef _IRQ_H
#define _IRQ_H

void enable_interrupt_controller(void);

extern "C" void handle_irq(void);
extern "C" void enable_irq(void);
extern "C" void disable_irq(void);

#endif /*_IRQ_H */