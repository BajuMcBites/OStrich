#ifndef _P_TIMER_H
#define _P_TIMER_H

#include "peripherals/base.h"

#define TIMER_CS (volatile unsigned int*)(PBASE + 0x00003000)
#define TIMER_CLO (volatile unsigned int*)(PBASE + 0x00003004)
#define TIMER_CHI (volatile unsigned int*)(PBASE + 0x00003008)
#define TIMER_C0 (volatile unsigned int*)(PBASE + 0x0000300C)
#define TIMER_C1 (volatile unsigned int*)(PBASE + 0x00003010)
#define TIMER_C2 (volatile unsigned int*)(PBASE + 0x00003014)
#define TIMER_C3 (volatile unsigned int*)(PBASE + 0x00003018)

// #define GIC_BASE        0x3F000000
#define GIC_DIST_BASE (PBASE + 0x1000)  // Distributor Base Address
#define GIC_CPU_BASE (PBASE + 0x2000)   // CPU Interface Base Addres

#define TIMER_CS_M0 (1 << 0)
#define TIMER_CS_M1 (1 << 1)
#define TIMER_CS_M2 (1 << 2)
#define TIMER_CS_M3 (1 << 3)

#endif /*_P_TIMER_H */