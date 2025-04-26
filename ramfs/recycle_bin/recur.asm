main:
  stp	x29, x30, [sp, #-0x10]!
  mov	x29, sp
  mov	w0, #0x100             // =29
  bl	recursion
  ldp	x29, x30, [sp], #0x10
  ret


recursion:
  mov	x29, sp
  stp	x29, x30, [sp, #-0x1F8]! //prev 0x1b0 -> 0x1F8
  str	w0, [sp, #0x1c]

  /* print x30 right after store */
  mov	x0, x0
  mov	x8, #0x13               // =19
  svc	#0x8

  mov	x0, x30
  mov	x8, #0x13               // =19
  svc	#0x8

  mov	x0, sp
  mov	x8, #0x13               // =19
  svc	#0x8

  ldr	w0, [sp, #0x1c]
  sub	w0, w0, #0x1

  cmp   w0, #0x0

  b.eq skip_recursion

  bl	recursion

skip_recursion:

  ldp	x29, x30, [sp], #0x1F8
  ldr	w0, [sp, #0x1c]


  /* print x30 right before return */
  mov	x0, x0
  mov	x8, #0x12               // =18
  svc	#0x8

  mov	x0, x30
  mov	x8, #0x12              // =19
  svc	#0x8

  mov	x0, sp
  mov	x8, #0x12               // =19
  svc	#0x8

  ret


