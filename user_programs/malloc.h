#ifndef _USER_MALLOC
#define _USER_MALLOC

typedef unsigned long size_t;

extern uint64_t _sbrk(int size);

void* malloc(size_t size);
void free(void* ptr);

#endif /* _USER_MALLOC */