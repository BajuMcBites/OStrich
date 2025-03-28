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
    // Set the timer value
    mov x0, #1
    lsl x0, x0, #5
    msr CNTP_TVAL_EL0, x0          // Set the timer value

    // Enable the timer and interrupt
    mov x0, #0x3                   // Enable timer and interrupt
    msr CNTP_CTL_EL0, x0           // Enable the timer
    ret

.globl gic_init
gic_init:
    ret