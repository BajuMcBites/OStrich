#ifndef _TIMER_H
#define _TIMER_H

void timer_init(void);
extern "C" void handle_timer_irq(void);

#endif /*_TIMER_H */