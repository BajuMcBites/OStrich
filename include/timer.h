#ifndef _TIMER_H
#define _TIMER_H


// forward declaration
template <typename T>
class Atomic;

extern Atomic<int> curr_interrupt;

void timer_init(void);
void core_timer_init(void);
void init_core_timer(void);
extern "C" void handle_timer_irq(void);

#endif /*_TIMER_H */