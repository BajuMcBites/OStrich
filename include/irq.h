#ifndef _IRQ_H
#define _IRQ_H

#include "sys.h"
void enable_interrupt_controller(void);
// void gic_init(void);

// extern "C" void handle_irq(struct KernelEntryFrame *frame);
extern "C" void enable_irq(void);
extern "C" void disable_irq(void);
extern "C" void rearm_timer(void);
extern "C" void gic_init(void);

#endif /*_IRQ_H */