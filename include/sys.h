#ifndef _SYS__H_
#define _SYS__H_

#include "stdint.h"
// KernelEntryFrame structure.
// X[0] - return value
struct KernelEntryFrame {
    union {
        /* X[0] ... X[5] - arguments
        once function is called, X[0] will be the return value.
        */
        uint64_t X[31];  // 0 ... 30
    };
};

extern "C" void syscall_handler(KernelEntryFrame* frame);

void invoke_handler(int opcode);

#endif