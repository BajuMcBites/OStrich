#include "stdint.h"
#include "malloc.h"


void* malloc(size_t size) {
    return (void*) _sbrk((size + 15) & ~15);
}

void free(void* ptr) {
    return;
}