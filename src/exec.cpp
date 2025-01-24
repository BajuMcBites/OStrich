#include "uart.h"
#include "printf.h"
#include "stdint.h"

/**
 * common exception handler
 */

extern "C" void exc_handler(unsigned long type, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far)
{
    // print out interruption type
    switch(type) {
        case 0: printf("Synchronous"); break;
        case 1: printf("IRQ"); break;
        case 2: printf("FIQ"); break;
        case 3: printf("SError"); break;
    }
    printf(": ");
    // decode exception type (some, not all. See ARM DDI0487B_b chapter D10.2.28)
    switch(esr>>26) {
        case 0b000000: printf("Unknown"); break;
        case 0b000001: printf("Trapped WFI/WFE"); break;
        case 0b001110: printf("Illegal execution"); break;
        case 0b010101: printf("System call"); break;
        case 0b100000: printf("Instruction abort, lower EL"); break;
        case 0b100001: printf("Instruction abort, same EL"); break;
        case 0b100010: printf("Instruction alignment fault"); break;
        case 0b100100: printf("Data abort, lower EL"); break;
        case 0b100101: printf("Data abort, same EL"); break;
        case 0b100110: printf("Stack alignment fault"); break;
        case 0b101100: printf("Floating point"); break;
        default: printf("Unknown"); break;
    }
    // decode data abort cause
    if(esr>>26==0b100100 || esr>>26==0b100101) {
        printf(", ");
        switch((esr>>2)&0x3) {
            case 0: printf("Address size fault"); break;
            case 1: printf("Translation fault"); break;
            case 2: printf("Access flag fault"); break;
            case 3: printf("Permission fault"); break;
        }
        switch(esr&0x3) {
            case 0: printf(" at level 0"); break;
            case 1: printf(" at level 1"); break;
            case 2: printf(" at level 2"); break;
            case 3: printf(" at level 3"); break;
        }
    }
    // dump registers
    printf(":\n  ESR_EL1 0x%X ELR_EL1 0x%X\n SPSR_EL1 0%xX FAR_EL1 0x%X\n",
           esr, elr, spsr, far);
           
    // no return from exception for now
    while(1);
}
