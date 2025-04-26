#include "mm.h"
#include "arm/sysregs.h"

.macro init_inner_table start_addr, temp
    add \temp, \start_addr, #PAGE_SIZE   
    lsr \temp, \temp, #12  
    and \temp, \temp, #0xFFFFFFFFF  
    lsl \temp, \temp, #12    
    orr \temp, \temp, #3 
    str \temp, [\start_addr]
.endm

.macro insert_pmd_entry str_addr, phys_bits, temp1, temp2
    // Calculate kernel region index (0x80000 >> 21)
    mov \temp2, #0x80000
    lsr \temp2, \temp2, #21
    cmp \phys_bits, \temp2
    b.ne 3f
    
    // Kernel region - Normal memory with atomic support
    lsl \temp1, \phys_bits, #21
    movz \temp2, 0x711  // AF=1, SH=3 (Inner), AP=1, AttrIndx=4 (Normal memory)
    orr \temp1, \temp1, \temp2
    b 2f

3:  // Calculate mailbox region (0x40000000 >> 21)
    mov \temp2, #0x40000000
    lsr \temp2, \temp2, #21
    cmp \phys_bits, \temp2
    b.ne 4f
    
    // Device memory for mailbox - use Device-nGnRnE
    lsl \temp1, \phys_bits, #21
    movz \temp2, #0x401  // AF=1, SH=0, AP=1, AttrIndx=0 (Device-nGnRnE)
    orr \temp1, \temp1, \temp2
    b 2f

4:  // Calculate framebuffer region bounds
    mov \temp2, #0x3C000000
    lsr \temp2, \temp2, #21
    cmp \phys_bits, \temp2
    b.lt 5f
    mov \temp2, #0x3F000000
    lsr \temp2, \temp2, #21
    cmp \phys_bits, \temp2
    b.ge 5f
    
    // Device memory for framebuffer - use Device-nGnRE
    lsl \temp1, \phys_bits, #21
    movz \temp2, #0x40d  // AF=1, SH=0, AP=1, AttrIndx=3 (Device-nGnRE)
    orr \temp1, \temp1, \temp2
    b 2f

5:  // Calculate peripheral region bounds
    mov \temp2, #0x3F000000
    lsr \temp2, \temp2, #21
    cmp \phys_bits, \temp2
    b.lt 6f
    mov \temp2, #0x40000000
    lsr \temp2, \temp2, #21
    cmp \phys_bits, \temp2
    b.ge 6f
    
    // Device memory for peripherals - use Device-nGnRnE
    lsl \temp1, \phys_bits, #21
    movz \temp2, #0x401  // AF=1, SH=0, AP=1, AttrIndx=0 (Device-nGnRnE)
    orr \temp1, \temp1, \temp2
    b 2f

6:  // Everything else - Normal memory with atomic support
    lsl \temp1, \phys_bits, #21
    movz \temp2, #0x711  // AF=1, SH=3 (Inner), AP=1, AttrIndx=4 (Normal memory)
    orr \temp1, \temp1, \temp2

2:  str \temp1, [\str_addr]
.endm


.macro init_pmd_table_entries start_addr, entry_num, temp1, temp2
    // First, map the kernel's physical address (0x80000) to its virtual address (0xffff000000080000)
    mov \entry_num, #0x47    // Index for 0xffff000000080000 >> 21
    mov \temp1, #0x80        // Physical address 0x80000 >> 21
    lsl \temp1, \temp1, #21
    movz \temp2, #0x711      // AF=1, SH=3 (Inner), AP=1, AttrIndx=4 (Normal memory)
    orr \temp1, \temp1, \temp2
    str \temp1, [\start_addr, \entry_num, lsl #3]

    // Now map the rest of memory
    movz \entry_num, #0
1:
    cmp \entry_num, #0x47
    b.eq 2f                  // Skip the kernel entry we already mapped
    insert_pmd_entry \start_addr, \entry_num, \temp1, \temp2
2:
    add \start_addr, \start_addr, #8
    add \entry_num, \entry_num, #1
    cmp \entry_num, #512
    blt 1b
.endm

.global create_page_tables
create_page_tables:
	mov    x28, x30

	// clear page tables to 0
	adrp    x0, __paging_start
	add     x0, x0, #:lo12:__paging_start
	adrp    x1, __paging_end
	add     x1, x1, #:lo12:__paging_end
	sub     x1, x1, x0
	bl      memzero

	adrp    x0, __paging_start
	add     x0, x0, #:lo12:__paging_start

	// set up PGD
	init_inner_table x0, x1

	// set up PUD
	add x0, x0, #PAGE_SIZE
	init_inner_table x0, x1

	// set up PMD entries
	add x0, x0, #PAGE_SIZE
	init_pmd_table_entries x0, x1, x2, x3

	mov x30, x28
	ret

.global init_mmu
init_mmu:
    dsb sy

    // base address of the page table into ttbr1_el1
    adrp x0, __paging_start
    add  x0, x0, #:lo12:__paging_start
    orr  x0, x0, #1
    msr  ttbr1_el1, x0
    msr  ttbr0_el1, x0 // optional if user space is mapped here
    isb

    // Translation Control Register
    ldr x0, =TCR_VALUE
    msr tcr_el1, x0
    isb

    // Memory Attribute Indirection Register
    ldr x0, =MAIR_VALUE
    msr mair_el1, x0

    // Update SP, FP, LR to virtual addresses
    ldr x4, =VA_START
    add sp, sp, x4
    add x29, x29, x4
    add x30, x30, x4

    // Invalidate TLB and caches
    tlbi vmalle1is
    dsb sy
    isb

    // Enable MMU
    mrs x0, sctlr_el1
    ldr x4, =SCTLR_MMU_ENABLED
    orr x0, x0, x4
    msr sctlr_el1, x0

    tlbi vmalle1is
    dsb sy
    isb

    ret


.global switch_ttbr0
switch_ttbr0:
    msr ttbr0_el1, x0
	ret

	