#ifndef _P_TIMER_H
#define _P_TIMER_H

#include "peripherals/base.h"

#define TIMER_CS (volatile unsigned int*)(PBASE + 0x00003000) // reserved
#define TIMER_CLO (volatile unsigned int*)(PBASE + 0x00003004) // counter low
#define TIMER_CHI (volatile unsigned int*)(PBASE + 0x00003008) // counter high
#define TIMER_C0 (volatile unsigned int*)(PBASE + 0x0000300C) // cmp 0
#define TIMER_C1 (volatile unsigned int*)(PBASE + 0x00003010) // cmp 1
#define TIMER_C2 (volatile unsigned int*)(PBASE + 0x00003014) // cmp 2
#define TIMER_C3 (volatile unsigned int*)(PBASE + 0x00003018) // cmp 3


#define TIMER_CS_M0 (1 << 0)
#define TIMER_CS_M1 (1 << 1)
#define TIMER_CS_M2 (1 << 2)
#define TIMER_CS_M3 (1 << 3)

#define SYSTEM_CLOCK_FREQ 1200000000  // 1.2 GHz

#endif /*_P_TIMER_H */