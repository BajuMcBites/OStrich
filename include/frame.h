#include "vm.h"
#include "stdint.h"

#define USED_PAGE_FLAG 0x1
#define PINNED_PAGE_FLAG 0x2

void create_frame_table(uintptr_t start, int size);

typedef struct Frame {
    int flags;
} Frame;
