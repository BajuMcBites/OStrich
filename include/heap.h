#ifndef _HEAP_H_
#define _HEAP_H_

#include "stdint.h"

extern void heapInit(void *start, size_t bytes);
extern "C" void *malloc(size_t size);
extern "C" void free(void *p);

#endif
