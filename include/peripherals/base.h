#ifndef _P_BASE_H
#define _P_BASE_H

#include "vm.h"

#define DEVICE_BASE 0x3F000000
#define ARM_DEVICE 0x40000000
#define PBASE DEVICE_BASE + VA_START
#define ARMBASE ARM_DEVICE + VA_START

#endif /*_P_BASE_H */