.globl enable_irq
enable_irq:
	msr    daifclr, #2 
	ret

.globl disable_irq
disable_irq:
	msr	daifset, #2
	ret

.globl rearm_timer
rearm_timer:
    mov x0, #1                    // Load 1 into x0
    lsl x0, x0, #20               // Shift left by 20 bits (1 << 20 == 1,000,000)
    msr CNTPS_TVAL_EL1, x0          // Re-load CNTPS_TVAL register

    // Re-enable the timer interrupt
    mov x0, #0x3                   // Enable timer and interrupt again
    msr CNTPS_CTL_EL1, x0           // Re-enable the timer
	ret