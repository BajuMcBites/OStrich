.globl memzero
memzero:
	str xzr, [x0], #8
	subs x1, x1, #8
	b.gt memzero
	ret

.macro init_inner_table start_addr, temp
    add \temp, \start_addr, #4096   
    lsr \temp, \temp, #11  
    and \temp, \temp, #0xFFFFFFFFF  
    lsl \temp, \temp, #11    
    orr \temp, \temp, #3     
    str \temp, [\start_addr]
.endm

.macro insert_pmd_entry str_addr, phys_bits, temp1, temp2
	and \temp1, \phys_bits, #0xFFFFFFF
	lsl \temp1, \phys_bits, #20
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
	mov    x29, x30

	// clear page tables to 0
	adr    x0, __paging_start
    adr    x1, __paging_end
    sub    x1, x1, x0
    bl     memzero

	//set up PGD -- ins addr: 0x809dc
	init_inner_table x0, x1

	//set up PUD
	add x0, x0, #4096
	init_inner_table x0, x1

	// set up PMD entries
	add x0, x0, #4096
	init_pmd_table_entries x0, x1, x2, x3

	mov x30, x29
	ret

	// update sp, load page table into register, turn on mmu
	// Define necessary constants
// .equ VA_START, 0xFFFF000000000000
// .equ LOW_MEMORY, 0x80000
// remove these values later:
.equ TCR_VALUE, 0x801010 
.equ MAIR_VALUE, 0x00004400
.equ SCTLR_MMU_ENABLED, 0x1
.equ VA_START, 0xFFFF000000000000

.global init_mmu
init_mmu:
    // Update stack pointer to use a virtual address
	movz x4, #0xFFFF
	lsl x4, x4, #48
	add sp, sp, x4

    //base address of the page table into ttbr1_el1
    adr   x0, __paging_start
    msr    ttbr1_el1, x0

    //Translation Control Register
    ldr    x0, =(TCR_VALUE)
    msr    tcr_el1, x0

    //Memory Attribute Indirection Register
    ldr    x0, =(MAIR_VALUE)
    msr    mair_el1, x0


	// load into return register
	add x30, x30, x4

    // Enable the MMU ?? (can't find the register)
    mov    x0, #SCTLR_MMU_ENABLED 
    msr    SCTLR_EL1, x0 //error here for some reason

	ret