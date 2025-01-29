.globl memzero
memzero:
	str xzr, [x0], #8
	subs x1, x1, #8
	b.gt memzero
	ret

.global create_page_tables
create_page_tables:
	mov    x29, x30

	// clear page tables to 0

	adr x0, __paging_start
	add x1, x0, #4096
	
	lsr	x1, x1, #11
	and x1, x1, #0xFFFFFFFFF
	lsl x1, x1, #11

	orr x1, x1, #3

	str x1, [x0]

	mov x30, x29
	ret

