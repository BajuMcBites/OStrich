main:
        sub     sp, sp, #48
        str     w0, [sp, 12]
        mov     w0, 1
        str     w0, [sp, 44]
        mov	x8, #0x4               // =19
        svc	#0x8
        mov	x8, #0x4               // =19
        svc	#0x8
        mov	x8, #0x4               // =19
        svc	#0x8
        cmp     x0, 0
        bne     .L2
        ldr     w0, [sp, 44]
        cmp     w0, 1
        bne     .L3
        ldr     w0, [sp, 44]
        cmp     w0, 2
        bne     .L4
.L3:
        str     xzr, [sp, 32]
        ldr     x0, [sp, 32]
        mov     w1, 1
        str     w1, [x0]
        b       .L1
.L4:
        mov     w0, 2
        str     w0, [sp, 44]
        mov	x8, #0x13               // =19
        svc	#0x8
        b       .L1
.L2:
        ldr     w0, [sp, 44]
        cmp     w0, 1
        bne     .L6
        ldr     w0, [sp, 44]
        cmp     w0, 2
        bne     .L7
.L6:
        str     xzr, [sp, 24]
        ldr     x0, [sp, 24]
        mov     w1, 1
        str     w1, [x0]
        b       .L1
.L7:
        mov     w0, 2
        str     w0, [sp, 44]
        mov	x8, #0x12               // =19
        svc	#0x8
.L1:
        add     sp, sp, 48
        mov	x8, #0x0               // =19
        svc	#0x8
        ret
