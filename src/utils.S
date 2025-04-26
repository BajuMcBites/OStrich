.globl put32
put32:
    str w1, [x0]
	ret

.globl get32
get32:
	ldr w0,[x0]
	ret

.globl delay
delay:
	subs x0, x0, #1
	bne delay
	ret

.globl getCoreID
getCoreID:
    mrs x0, mpidr_el1       // Move the value of MPIDR_EL1 into x0
    and x0, x0, #0xFF       // Mask to get the lower 8 bits, which represent the core ID
    ret                     // Return from the function

.globl get_el
	get_el:
	mrs x0, CurrentEL
	lsr x0, x0, #2
	ret

.global get_sp
    get_sp:
    mov x0, sp
    ret

.global memset
.type memset, %function
memset:
    // Arguments: x0 = ptr, x1 = value, x2 = num
    // Return value: x0 (same pointer as input)

    // Store the value to be set in x3 for later use
    mov     x3, x1          // x3 = value (to be used for filling)

    // Check if num (x2) is zero
    cbz     x2, memset_done  // If num == 0, exit early

memset_loop:
    strb    w3, [x0], #1     // Store byte (w3) to address in x0, increment pointer
    subs    x2, x2, #1       // Decrement num (x2)
    bne     memset_loop      // If num != 0, loop again

memset_done:
    ret

.global set_sp_and_jump
.type set_sp_and_jump, %function
set_sp_and_jump:
    mov sp, x0     // Set stack pointer to first argument
    br  x1         // Branch to function pointer (no return)

.globl flush_tlb
flush_tlb:
  isb
  tlbi vmalle1
  isb
  ret