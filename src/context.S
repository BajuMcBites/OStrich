.globl context_switch
context_switch:
    # x0 = save_old_sp_here (pointer to TCB of current thread)
    # x1 = restore_new_sp_from_here (pointer to TCB of next thread)

    # Save the callee-saved registers
    str x19, [sp, #-8]!
    str x20, [sp, #-8]!
    str x21, [sp, #-8]!
    str x22, [sp, #-8]!
    str x23, [sp, #-8]!
    str x24, [sp, #-8]!
    str x25, [sp, #-8]!
    str x26, [sp, #-8]!
    str x27, [sp, #-8]!
    str x28, [sp, #-8]!
    str x29, [sp, #-8]!
    str lr, [sp, #-8]!    

    # Save the current sp in the TCB (this saves the old thread's state)
   #  mov x2, sp
   # str x2, [x0]          

    # Load the new sp from the next thread's TCB
    ldr x2, [x1]          
    mov sp, x2        
    # Restore callee-saved registers
    ldr lr, [sp], #8
    ldr x29, [sp], #8
    ldr x28, [sp], #8
    ldr x27, [sp], #8
    ldr x26, [sp], #8
    ldr x25, [sp], #8
    ldr x24, [sp], #8
    ldr x23, [sp], #8
    ldr x22, [sp], #8
    ldr x21, [sp], #8
    ldr x20, [sp], #8
    ldr x19, [sp], #8    

    ret

.globl cpu_switch_to
cpu_switch_to:
	mov	x10, 0
	add	x8, x0, x10
	mov	x9, sp
	stp	x19, x20, [x8], #16		
	stp	x21, x22, [x8], #16
	stp	x23, x24, [x8], #16
	stp	x25, x26, [x8], #16
	stp	x27, x28, [x8], #16
	stp	x29, x9, [x8], #16
	str	x30, [x8]
	add	x8, x1, x10
	ldp	x19, x20, [x8], #16		
	ldp	x21, x22, [x8], #16
	ldp	x23, x24, [x8], #16
	ldp	x25, x26, [x8], #16
	ldp	x27, x28, [x8], #16
	ldp	x29, x9, [x8], #16
	ldr	x30, [x8]
	mov	sp, x9
	ret