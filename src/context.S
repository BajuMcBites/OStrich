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
    str x30, [sp, #-8]!    

    # Save the current sp in the TCB (this saves the old thread's state)
    mov x2, sp
    # Store the current SP in TCB

    str x2, [x0]          
    # Load the new sp from the next thread's TCB
    ldr x2, [x1]          
    # mov sp, x2           

    # Restore callee-saved registers
    ldr x30, [x2], #8
    ldr x29, [x2], #8
    ldr x28, [x2], #8
    ldr x27, [x2], #8
    ldr x26, [x2], #8
    ldr x25, [x2], #8
    ldr x24, [x2], #8
    ldr x23, [x2], #8
    ldr x22, [x2], #8
    ldr x21, [x2], #8
    ldr x20, [x2], #8
    ldr x19, [x2], #8    
    ret