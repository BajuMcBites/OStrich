#include "mm.h"
#include "arm/sysregs.h"

//---------------------------------------------------------
// Macro: init_inner_table
// Sets up a table descriptor pointing to the next-level table.
//---------------------------------------------------------
.macro init_inner_table start_addr, temp
    add     \temp, \start_addr, #PAGE_SIZE   
    lsr     \temp, \temp, #12  
    and     \temp, \temp, #0xFFFFFFFFF  
    lsl     \temp, \temp, #12    
    orr     \temp, \temp, #3 
    str     \temp, [\start_addr]
.endm

//---------------------------------------------------------
// Macro: insert_pmd_entry
// Fills one PMD entry for lower addresses (below 0x40000000)
// based on various region tests.
//---------------------------------------------------------
.macro insert_pmd_entry str_addr, phys_bits, temp1, temp2
    // Compare with kernel region index (0x80000 >> 21)
    mov     \temp2, #0x80000
    lsr     \temp2, \temp2, #21
    cmp     \phys_bits, \temp2
    b.ne    3f
    
    // Kernel region: normal memory with atomic support.
    lsl     \temp1, \phys_bits, #21
    movz    \temp2, #0x711   // AF=1, SH=3 (Inner), AP=1, AttrIndx=4.
    orr     \temp1, \temp1, \temp2
    b       2f

3:  // Check for mailbox region: 0x40000000 >> 21.
    mov     \temp2, #0x40000000
    lsr     \temp2, \temp2, #21
    cmp     \phys_bits, \temp2
    b.ne    4f
    
    // Mailbox region: map as device memory.
    lsl     \temp1, \phys_bits, #21
    movz    \temp2, #0x40d   // Device attributes (adjust as needed).
    orr     \temp1, \temp1, \temp2
    b       2f

4:  // Framebuffer region bounds:
    mov     \temp2, #0x3C000000
    lsr     \temp2, \temp2, #21
    cmp     \phys_bits, \temp2
    b.lt    5f
    mov     \temp2, #0x3F000000
    lsr     \temp2, \temp2, #21
    cmp     \phys_bits, \temp2
    b.ge    5f
    // Framebuffer: map as device memory.
    lsl     \temp1, \phys_bits, #21
    movz    \temp2, #0x40d   // Device attributes.
    orr     \temp1, \temp1, \temp2
    b       2f

5:  // Peripheral region bounds:
    mov     \temp2, #0x3F000000
    lsr     \temp2, \temp2, #21
    cmp     \phys_bits, \temp2
    b.lt    6f
    mov     \temp2, #0x40000000
    lsr     \temp2, \temp2, #21
    cmp     \phys_bits, \temp2
    b.ge    6f
    // Peripherals: map as device memory.
    lsl     \temp1, \phys_bits, #21
    movz    \temp2, #0x401
    orr     \temp1, \temp1, \temp2
    b       2f

6:  // Default: map as normal memory with atomic support.
    lsl     \temp1, \phys_bits, #21
    movz    \temp2, #0x711
    orr     \temp1, \temp1, \temp2

2:  str     \temp1, [\str_addr]
.endm

//---------------------------------------------------------
// Macro: init_pmd_table_entries
// Fills a PMD table for addresses below 0x40000000.
//---------------------------------------------------------
.macro init_pmd_table_entries start_addr, entry_num, temp1, temp2
    // Map the kernel's physical address (0x80000) to its virtual address.
    mov     \entry_num, #0x47    // Kernel mapping index.
    mov     \temp1, #0x80        // 0x80000 >> 21 equals 0x80.
    lsl     \temp1, \temp1, #21
    movz    \temp2, #0x711       // Normal memory attributes.
    orr     \temp1, \temp1, \temp2
    str     \temp1, [\start_addr, \entry_num, lsl #3]

    // Map the remaining 512 entries.
    movz    \entry_num, #0
1:
    cmp     \entry_num, #0x47
    b.eq    2f                  // Skip the kernel entry.
    insert_pmd_entry \start_addr, \entry_num, \temp1, \temp2
2:
    add     \start_addr, \start_addr, #8
    add     \entry_num, \entry_num, #1
    cmp     \entry_num, #512
    blt     1b
.endm

//---------------------------------------------------------
// Macro: init_pmd_table_entries_device
// Fills the second PMD table (PMD2) for addresses >= 0x40000000,
// mapping every entry with device attributes.
//---------------------------------------------------------
.macro init_pmd_table_entries_device start_addr, entry_num, temp1, temp2
    movz    \entry_num, #0
1:
    lsl     \temp1, \entry_num, #21    
    // Load constant 0x40000000 into temp2
    movz    \temp2, #0x4000, lsl #16    // 0x4000 << 16 == 0x40000000
    add     \temp1, \temp1, \temp2       // Add base address to the offset
    // Now load device memory attribute constant into temp2
    movz    \temp2, #0x401              // Device memory attribute.
    orr     \temp1, \temp1, \temp2       // OR in attribute bits.
    str     \temp1, [\start_addr, \entry_num, lsl #3]
    add     \entry_num, \entry_num, #1
    cmp     \entry_num, #512
    blt     1b
.endm


//---------------------------------------------------------
// Global: create_page_tables
// Sets up the page table hierarchy (PGD → PUD → PMD/PMD2).
//---------------------------------------------------------
.global create_page_tables
create_page_tables:
    mov     x28, x30

    // Clear page table memory.
    adr     x0, __paging_start
    adr     x1, __paging_end
    sub     x1, x1, x0
    bl      memzero

    // Set up PGD (first level).
    adr     x0, __paging_start    // PGD base.
    init_inner_table x0, x1       // PGD[0] now points to PUD.

    // Set up PUD (second level).
    add     x0, x0, #PAGE_SIZE     // x0 now points to the PUD.
    init_inner_table x0, x1       // This initializes PUD[0] to point to PMD.

    // Patch PUD[1] to point to PMD2.
    // Compute address of PUD[1]:
    adr     x2, __paging_start    // PGD base.
    add     x2, x2, #PAGE_SIZE     // x2 now = PUD base.
    add     x2, x2, #8            // x2 now points to PUD[1] (each entry is 8 bytes).
    adr     x3, PMD_arm              // x3 = address of PMD2.
    and     x3, x3, #~0xFFF       // Clear lower 12 bits.
    orr     x3, x3, #3            // Set table descriptor flags.
    str     x3, [x2]              // Write PMD2 descriptor into PUD[1].

    // Set up lower PMD table (for addresses below 0x40000000).
    adr     x0, PMD
    init_pmd_table_entries x0, x1, x2, x3

    // Set up upper PMD table (for addresses >= 0x40000000) with device mapping.
    adr     x0, PMD_arm
    init_pmd_table_entries_device x0, x1, x2, x3


    mov     x30, x28
    ret

//---------------------------------------------------------
// Global: init_mmu
// Initializes the MMU (loads page table base registers, TCR, MAIR, etc.).
//---------------------------------------------------------
.global init_mmu
init_mmu:
    dsb     sy
    // Set the base address of the page tables into ttbr1_el1 and ttbr0_el1.
    adr     x0, __paging_start
    orr     x0, x0, #1
    msr     ttbr1_el1, x0
    msr     ttbr0_el1, x0
    isb

    // Set the Translation Control Register.
    ldr     x0, =(TCR_VALUE)
    msr     tcr_el1, x0
    isb

    // Set the Memory Attribute Indirection Register.
    ldr     x0, =(MAIR_VALUE)
    msr     mair_el1, x0

    // Adjust stack pointer and frame pointer for virtual addressing.
    ldr     x4, =(VA_START)
    add     sp, sp, x4
    add     x29, x29, x4
    add     x30, x30, x4

    // Invalidate TLB and caches.
    tlbi    vmalle1is
    dsb     sy
    isb

    // Enable the MMU and data cache.
    mrs     x0, sctlr_el1
    ldr     x4, =(SCTLR_MMU_ENABLED)
    orr     x0, x0, x4
    msr     sctlr_el1, x0

    tlbi    vmalle1is
    dsb     sy
    isb

    ret

//---------------------------------------------------------
// Global: switch_ttbr0
// Switches TTBR0 to a new page table base.
//---------------------------------------------------------
.global switch_ttbr0
switch_ttbr0:
    msr     ttbr0_el1, x0
    ret
