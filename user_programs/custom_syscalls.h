#ifndef _CUSTOM_SYSCALLS
#define _CUSTOM_SYSCALLS

#include "stdint.h"

long time_elapsed();
int  sys_draw_frame(void * rendered_frame);
int  checkpoint();
int  mount(uint32_t checkpoint_id);
int  ls();
int  del(char* path);

#endif