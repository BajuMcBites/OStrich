#include "vm.h"
#include "stdint.h"
#include "event_loop.h"
#include "event.h"

#define USED_PAGE_FLAG 0x1
#define PINNED_PAGE_FLAG 0x2

void create_frame_table(uintptr_t start, int size);

template <typename work>
void alloc_frame(int flags, work w);

typedef struct Frame {
    int flags;
} Frame;
