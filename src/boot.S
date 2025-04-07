#include "arm/sysregs.h"
#include "vm.h"
#include "entry.h"
#include "mm.h"
#include "sys.h"

.section ".text.boot"
_start:	
    mrs x0, mpidr_el1
    and x0, x0, #0xFF        // Check processor id
    cbz x0, master           // Continue for primary CPU
    b proc_hang              // Hang for all non-primary CPUs

proc_hang:
    wfe
    b proc_hang

master:
    // Set up stack pointer for EL1
    mrs x0, CurrentEL
    mov sp, #LOW_MEMORY
    bl pickKernelStack
    msr sp_el1, x0


    // Enable virtual and physical counter timers for EL1
    mrs     x0, cnthctl_el2
    orr     x0, x0, #3
    msr     cnthctl_el2, x0
    msr     cntvoff_el2, xzr

    // Disable coprocessor traps in EL2
    mov     x0, #0x33FF
    msr     cptr_el2, x0
    msr     hstr_el2, xzr
    mrs x0, CPACR_EL1         // Read the current value of CPACR_EL1
    orr x0, x0, #(0b01 << 20) // Set bits 20-21 to enable full access to FP/SIMD
    msr CPACR_EL1, x0         // Write the value back to CPACR_EL1

    // Configure EL2 (Hypervisor)
    ldr x0, =HCR_VALUE
    msr hcr_el2, x0

    ldr     x0, =_vectors
    msr     vbar_el1, x0

    // Set up SPSR_EL2 to transition to EL1h
    ldr x0, =SPSR_VALUE
    msr spsr_el2, x0

    // Set ELR_EL2 to point to el1_entry
    adr x0, el1_entry
    msr elr_el2, x0

    // Transition to EL1
    eret

el1_entry:
    // Clear BSS section
    adr    x0, __bss_start
    adr    x1, __bss_end
    sub    x1, x1, x0
    bl     memzero

    bl    primary_kernel_init
    b     proc_hang        // should never come here

secondary:
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    mov x1, #SECTION_SIZE
    mul x1, x1, x0
    add x1, x1, #LOW_MEMORY
    mov sp, x1

    // Set up stack pointer for EL1 for secondary cores
    bl pickKernelStack
    msr sp_el1, x0

setup_el1_for_secondary:

    // Enable virtual and physical counter timers for EL1
    mrs     x0, cnthctl_el2
    orr     x0, x0, #3
    msr     cnthctl_el2, x0
    msr     cntvoff_el2, xzr

    // Disable coprocessor in EL2
    mov     x0, #0x33FF
    msr     cptr_el2, x0
    msr     hstr_el2, xzr
    mov     x0, #(3 << 20)
    msr     cpacr_el1, x0


    // Configure EL2 (Hypervisor)
    ldr x0, =HCR_VALUE
    msr hcr_el2, x0

    // Set up SPSR_EL2 to transition to EL1h
    ldr x0, =SPSR_VALUE
    msr spsr_el2, x0

    // Set ELR_EL2 to point to secondary_kernel_main
    adr x0, secondary_kernel_main
    msr elr_el2, x0

    ldr     x0, =_vectors
    msr     vbar_el1, x0

    // Transition to EL1
    eret

.macro	kernel_entry
	sub	sp, sp, #S_FRAME_SIZE
	stp	x0, x1, [sp, #16 * 0]
	stp	x2, x3, [sp, #16 * 1]
	stp	x4, x5, [sp, #16 * 2]
	stp	x6, x7, [sp, #16 * 3]
	stp	x8, x9, [sp, #16 * 4]
	stp	x10, x11, [sp, #16 * 5]
	stp	x12, x13, [sp, #16 * 6]
	stp	x14, x15, [sp, #16 * 7]
	stp	x16, x17, [sp, #16 * 8]
	stp	x18, x19, [sp, #16 * 9]
	stp	x20, x21, [sp, #16 * 10]
	stp	x22, x23, [sp, #16 * 11]
	stp	x24, x25, [sp, #16 * 12]
	stp	x26, x27, [sp, #16 * 13]
	stp	x28, x29, [sp, #16 * 14]
	str	x30, [sp, #16 * 15] 
	.endm

	.macro	kernel_exit
	ldp	x0, x1, [sp, #16 * 0]
	ldp	x2, x3, [sp, #16 * 1]
	ldp	x4, x5, [sp, #16 * 2]
	ldp	x6, x7, [sp, #16 * 3]
	ldp	x8, x9, [sp, #16 * 4]
	ldp	x10, x11, [sp, #16 * 5]
	ldp	x12, x13, [sp, #16 * 6]
	ldp	x14, x15, [sp, #16 * 7]
	ldp	x16, x17, [sp, #16 * 8]
	ldp	x18, x19, [sp, #16 * 9]
	ldp	x20, x21, [sp, #16 * 10]
	ldp	x22, x23, [sp, #16 * 11]
	ldp	x24, x25, [sp, #16 * 12]
	ldp	x26, x27, [sp, #16 * 13]
	ldp	x28, x29, [sp, #16 * 14]
	ldr	x30, [sp, #16 * 15] 
	add	sp, sp, #S_FRAME_SIZE		
	eret
	.endm

secondary_kernel_main:
    bl secondary_kernel_init
    b proc_hang

.global wake_up_cores
wake_up_cores:
    
    mov x0, #0
    adr x0, secondary 
    ldr x4, =(VA_START)
    sub x0, x0, x4

    dsb sy
    isb

    mov x1, #0xe0
    str x0, [x1]
    mov x1, #0xe8
    str x0, [x1]
    mov x1, #0xf0
    str x0, [x1]

    // we wrote to addresses 0xe0, 0xe8, and 0xf0, we must now write them back to memory since 
    // primary core has data cache on at this point
    movz x0, #0xe0
    dc cvac, x0
    movz x0, #0xe8
    dc cvac, x0
    movz x0, #0xf0
    dc cvac, x0
    
    dsb sy
    isb
    
    sev
    
    ret



.macro ventry label
.align	7
b	\label
.endm


    // important, code has to be properly aligned
    .align 11
_vectors:
    ventry synchronous_el0
    ventry irq_el0
    ventry fiq_el0
    ventry serror_el0
    
    ventry synchronous_el1
    ventry irq_el1
    ventry fiq_el1
    ventry serror_el1
    
    ventry synchronous_el0_64
    ventry irq_el0_64
    ventry fiq_el0_64
    ventry serror_el0_64
    
    ventry synchronous_el0_32
    ventry irq_el0_32
    ventry fiq_el0_32
    ventry serror_el0_32

.macro handle_exception
mrs     x1, esr_el1
mrs     x2, elr_el1
mrs     x3, spsr_el1
mrs     x4, far_el1
b       exc_handler
.endm

synchronous_el0:
    mrs     x1, esr_el1         // ESR_EL1 has EC in bits [31:26]
    lsr     x2, x1, #26         // x2 = EC = ESR_EL1[31:26]
    cmp     x2, #0x15           // 0x15 = SVC from EL0
    b.ne    not_syscall         // if not SVC, go handle as exception

    // It's an SVC -> syscall
    kernel_entry
    mov x0, sp
    bl syscall_handler
    kernel_exit                 // returns to user via eret

not_syscall:
    mov     x0, #0              // you could pass reason info here if needed
    handle_exception            // handle exception (which jumps to exc_handler)

irq_el0: 
	kernel_entry 
	bl	handle_irq
	kernel_exit 

fiq_el0:
    mov     x0, #2
    handle_exception

serror_el0:
    mov     x0, #3
    handle_exception
    

// set up this way to test svc calls while in el1
synchronous_el1:
    mrs     x1, esr_el1         // ESR_EL1 has EC in bits [31:26]
    lsr     x2, x1, #26         // x2 = EC = ESR_EL1[31:26]
    cmp     x2, #0x15           // 0x15 = SVC from EL0
    b.ne    not_syscall         // if not SVC, go handle as exception

    // It's an SVC -> syscall
    kernel_entry
    bl syscall_handler
    kernel_exit                 // returns to user via eret

irq_el1: 
	kernel_entry 
	bl	handle_irq
	kernel_exit 

fiq_el1:
    mov     x0, #2
    handle_exception

serror_el1:
    mov     x0, #3
    handle_exception
    
synchronous_el0_64:
    mov     x0, #0
    handle_exception

irq_el0_64: 
	kernel_entry 
	bl	handle_irq
	kernel_exit 

fiq_el0_64:
    mov     x0, #2
    handle_exception

serror_el0_64:
    mov     x0, #3
    handle_exception
    
synchronous_el0_32:
    mov     x0, #0
    handle_exception

irq_el0_32: 
	kernel_entry 
	bl	handle_irq
	kernel_exit 

fiq_el0_32:
    mov     x0, #2
    handle_exception

serror_el0_32:
    mov     x0, #3
    handle_exception

.globl ret_from_fork
ret_from_fork:
	bl	schedule_tail
	mov	x0, x20
	blr	x19 		//should never return