#include "printf.h"

extern "C" {
unsigned int get_current_el(void) {
    unsigned long current_el;
    __asm__ volatile("mrs %0, CurrentEL" : "=r"(current_el));
    return (unsigned int)(current_el >> 2);
}

// Assembly to get syscall # and args and pass it to a C function to actual handle the call.
void svc_handler(void) {
    __asm(
        ".global svc_handler_main\n"
        "stp x29, x30, [sp, #-16]!\n"  // Save frame pointer and link register
        "mov x29, sp\n"                // Set up frame pointer
        "mov x0, sp\n"                 // Pass stack pointer to handler
        "bl svc_handler_main\n"        // Call C handler
        "ldp x29, x30, [sp], #16\n"    // Restore frame pointer and link register
        "ret\n"                        // Return
    );
}

// Actual handler.
void svc_handler_main(unsigned long *svc_args) {
    unsigned int svc_number;
    /*
     * In AArch64, SVC number is in the instruction itself
     * We can extract it from memory at (LR - 4)
     */
    unsigned long *return_addr =
        (unsigned long *)svc_args[30];  // x30 (LR) contains the return address
    svc_number = (*(unsigned int *)((unsigned char *)return_addr - 4)) &
                 0xFFFF;  // Extract SVC number from instruction

    printf("Handling syscall with opcode: %d\n", svc_number);
    // ARM Syscall Table Default: https://blog.xhyeax.com/2022/04/28/arm64-syscall-table/.
    switch (svc_number) {
        case 0: /* EnablePrivilegedMode */
            printf("case 0\n");
            // Privilege level changes are different in AArch64
            // Would need EL1/EL0 transition code here
            break;
        default: /* unknown SVC */
            break;
    }
}

// Dummy function that calls "SVC" with an opcode of your choice.
// We don't have code that runs in EL0 yet (to my knowledge), so this function is used instead.
void invoke_handler(int opcode) {
    __asm__ volatile(
        "mov x0, %0\n"
        "svc #0"
        :
        : "r"(opcode)
        : "x0");
}
}