#ifndef _IRQ_H
#define _IRQ_H

void enable_interrupt_controller(void);
void gic_init(void);

extern "C" void handle_irq(void);
extern "C" void enable_irq(void);
extern "C" void disable_irq(void);
extern "C" void rearm_timer(void);

#endif /*_IRQ_H */