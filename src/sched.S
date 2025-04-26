.globl load_user_context
load_user_context:

	ldr x1, [x0, #8 * 31]
	ldr x2, [x0, #8 * 32]
	ldr x3, [x0, #8 * 33]

	msr sp_el0, x1
	msr elr_el1, x2
	msr spsr_el1, x3

	ldp	x2, x3, [x0, #16 * 1]
	ldp	x4, x5, [x0, #16 * 2]
	ldp	x6, x7, [x0, #16 * 3]
	ldp	x8, x9, [x0, #16 * 4]
	ldp	x10, x11, [x0, #16 * 5]
	ldp	x12, x13, [x0, #16 * 6]
	ldp	x14, x15, [x0, #16 * 7]
	ldp	x16, x17, [x0, #16 * 8]
	ldp	x18, x19, [x0, #16 * 9]
	ldp	x20, x21, [x0, #16 * 10]
	ldp	x22, x23, [x0, #16 * 11]
	ldp	x24, x25, [x0, #16 * 12]
	ldp	x26, x27, [x0, #16 * 13]
	ldp	x28, x29, [x0, #16 * 14]
	ldr	x30, [x0, #16 * 15]

	ldp	x0, x1, [x0, #16 * 0]
	eret

.globl get_sp_el0
get_sp_el0:
	mrs x0, sp_el0
	ret

.globl get_elr_el1
get_elr_el1:
	mrs x0, elr_el1
	ret

.globl get_spsr_el1
get_spsr_el1:
	mrs x0, spsr_el1
	ret

.globl get_far_el1
get_far_el1:
	mrs x0, far_el1
	ret

.global get_esr_el1
get_esr_el1:
	mrs x0, esr_el1
	ret