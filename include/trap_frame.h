#ifndef _TRAP_FRAME_H
#define _TRAP_FRAME_H

#include "stdint.h"

struct trap_frame {
    uint64_t X[31];
};

typedef trap_frame SyscallFrame;

#endif /* _TRAP_FRAME_H */