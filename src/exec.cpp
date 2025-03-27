#include "atomic.h"
#include "irq.h"
#include "printf.h"
#include "stdint.h"
#include "uart.h"

/**
 * common exception handler
 */

SpinLock lockErr;

extern "C" void exc_handler(unsigned long type, unsigned long esr, unsigned long elr,
                            unsigned long spsr, unsigned long far) {
    lockErr.lock();
    // print out interruption type
    switch (type) {
        case 0:
            printf_err("Synchronous");
            break;
        case 1:
            printf_err("IRQ");
            break;
        case 2:
            printf_err("FIQ");
            break;
        case 3:
            printf_err("SError");
            break;
    }
    printf_err(": ");
    // decode exception type (some, not all. See ARM DDI0487B_b chapter D10.2.28)
    switch (esr >> 26) {
        case 0b000000:
            printf_err("Unknown");
            break;
        case 0b000001:
            printf_err("Trapped WFI/WFE");
            break;
        case 0b001110:
            printf_err("Illegal execution");
            break;
        case 0b010101:
            printf_err("System call");
            break;
        case 0b100000:
            printf_err("Instruction abort, lower EL");
            break;
        case 0b100001:
            printf_err("Instruction abort, same EL");
            break;
        case 0b100010:
            printf_err("Instruction alignment fault");
            break;
        case 0b100100:
            printf_err("Data abort, lower EL");
            break;
        case 0b100101:
            printf_err("Data abort, same EL");
            break;
        case 0b100110:
            printf_err("Stack alignment fault");
            break;
        case 0b101100:
            printf_err("Floating point");
            break;
        default:
            printf_err("Unknown");
            break;
    }
    // decode data abort cause
    if (esr >> 26 == 0b100100 || esr >> 26 == 0b100101) {
        printf_err(", ");
        switch ((esr >> 2) & 0x3) {
            case 0:
                printf_err("Address size fault");
                break;
            case 1:
                printf_err("Translation fault");
                break;
            case 2:
                printf_err("Access flag fault");
                break;
            case 3:
                printf_err("Permission fault");
                break;
        }
        switch (esr & 0x3) {
            case 0:
                printf_err(" at level 0");
                break;
            case 1:
                printf_err(" at level 1");
                break;
            case 2:
                printf_err(" at level 2");
                break;
            case 3:
                printf_err(" at level 3");
                break;
        }
    }
    // dump registers
    printf_err(":\n  ESR_EL1 0x%X ELR_EL1 0x%X\n SPSR_EL1 0%xX FAR_EL1 0x%X\n", esr, elr, spsr,
               far);
    lockErr.unlock();

    // no return from exception for now
    while (1);
}
