#ifndef _TIMER_H
#define _TIMER_H

#include "stdint.h"

void timer_init(void);
void local_timer_init(void);
extern "C" void handle_timer_irq(void);
void wait_msec(unsigned int);
uint64_t get_systime();

#endif /*_TIMER_H */