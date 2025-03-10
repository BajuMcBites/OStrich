#include "vm.h"
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
	and \temp1, \phys_bits, #0xFFFFFFF
	lsl \temp1, \phys_bits, #21
	movz \temp2, #0x405 // set flags: no cache (3rd bit is 1), mm access (11th bit), valid type (1st bit)
	orr \temp1, \temp1, \temp2 
	str \temp1, [\str_addr]
.endm

.macro init_pmd_table_entries start_addr, entry_num, temp1, temp2
	movz \entry_num, #0
1:
	insert_pmd_entry \start_addr, \entry_num, \temp1, \temp2
	add \start_addr, \start_addr, #8
	add \entry_num, \entry_num, #1
	cmp \entry_num, #512
	blt 1b
.endm

.global create_page_tables
create_page_tables:
	mov    x28, x30

	// clear page tables to 0
	adr    x0, __paging_start
    adr    x1, __paging_end
    sub    x1, x1, x0
    bl     memzero

	adr    x0, __paging_start

	//set up PGD -- ins addr: 0x809dc
	init_inner_table x0, x1

	//set up PUD
	add x0, x0, #PAGE_SIZE
	init_inner_table x0, x1

	// set up PMD entries
	add x0, x0, #PAGE_SIZE
	init_pmd_table_entries x0, x1, x2, x3

	mov x30, x28
	ret

.global init_mmu
init_mmu:
    // Update stack pointer to use a virtual address
	
    //base address of the page table into ttbr1_el1
    adr x0, __paging_start
	orr x0, x0, #1
    msr ttbr1_el1, x0
    msr ttbr0_el1, x0 

    //Translation Control Register
    ldr x0, =(TCR_VALUE)
    msr tcr_el1, x0

    //Memory Attribute Indirection Register
    ldr x0, =(MAIR_VALUE)
    msr mair_el1, x0

	// load into return register
	ldr x4, =(VA_START)
	add sp, sp, x4
	add x29, x29, x4
	add x30, x30, x4

    // Enable the MMU
	mrs x0, sctlr_el1
    orr x0, x0, #SCTLR_MMU_ENABLED 
    msr sctlr_el1, x0 
	
	ret

.global switch_ttbr0
switch_ttbr0:
    msr ttbr0_el1, x0
	ret

	