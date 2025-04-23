#ifndef _TIMER_H
#define _TIMER_H

void timer_init(void);
void local_timer_init(void);
extern "C" void handle_timer_irq(void);
void wait_msec(unsigned int);

#endif /*_TIMER_H */