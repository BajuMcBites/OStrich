#ifndef _CUSTOM_SYSCALLS
#define _CUSTOM_SYSCALLS

#include "stdint.h"

long time_elapsed();
int  sys_draw_frame(void * rendered_frame);

#endif