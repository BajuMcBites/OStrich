#include "sys.h"

#include "stdint.h"

extern "C" void syscall_handler(void* syscall_frame) {
    struct SyscallFrame {
        uint64_t x0, x1, x2, x3, x4, x5, x6, x7;
        uint64_t x8, x9, x10, x11, x12, x13, x14, x15;
        uint64_t x16, x17, x18, x19, x20, x21, x22, x23;
        uint64_t x24, x25, x26, x27, x28, x29;
        uint64_t x30;  // LR
    };

    auto* frame = (SyscallFrame*)(syscall_frame);

    switch (frame->x8) {  // syscall number
        case SYS_GETPID:
            break;
        case SYS_WRITE:
            // handle write
            break;
        default:
            frame->x0 = -1;  // error
            break;
    }
}