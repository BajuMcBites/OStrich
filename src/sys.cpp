#include "sys.h"

#include "printf.h"
#include "stdint.h"
#include "vm.h"

/**Frame pointing to all of the registers. These will be
 * pushed back into the registers upon returning from the
 * system call handler. Linux convention has the system call
 * number in x8 and registers x0 - x5 are used for arguments
 * For ARM, x0 is used to return a value.**/

void sys_breakpoint() {
    return;
}

void invoke_handler(int opcode) {
    __asm__ volatile(
        "mov x8, %0\n"  // Put syscall number into x8
        "svc #0\n"
        :
        : "r"(opcode)  // Tell compiler to put 'opcode' into a register
        : "x8");
}

void syscall_handler(void* syscall_frame) {
    sys_breakpoint();
    struct SyscallFrame {
        uint64_t x0, x1, x2, x3, x4, x5, x6, x7;
        uint64_t x8, x9, x10, x11, x12, x13, x14, x15;
        uint64_t x16, x17, x18, x19, x20, x21, x22, x23;
        uint64_t x24, x25, x26, x27, x28, x29;
        uint64_t x30;  // LR
    };

    auto* frame = (SyscallFrame*)(syscall_frame);
    sys_breakpoint();
    printf("hello in the sys handler.\n");
    printf("sp passed in: 0x%x%x\n", (uint64_t)syscall_frame >> 32, syscall_frame);
    printf("call numb: %d\n", frame->x8);

    switch (frame->x8) {  // syscall number
        case SYS_EXIT:
            break;
        case SYS_CLOSE:
            break;
        case SYS_ENV:
            break;
        case SYS_EXEC:
            break;
        case SYS_FORK:
            break;
        case SYS_FSTAT:
            break;
        case SYS_GETPID:
            break;
        case SYS_ISATTY:
            break;
        case SYS_KILL:
            break;
        case SYS_LINK:
            break;
        case SYS_LSEEK:
            break;
        case SYS_OPEN:
            break;
        case SYS_READ:
            break;
        case SYS_STAT:
            break;
        case SYS_UNLINK:
            break;
        case SYS_WAIT:
            break;
        case SYS_WRITE:
            break;
        case SYS_TIME:
            break;
        default:
            frame->x0 = -1;  // error
            break;
    }
}