#ifndef	_MM_H
#define	_MM_H

#define VA_START 			0xffff000000000000

#define PAGE_SHIFT	 		12
#define TABLE_SHIFT 			9
#define SECTION_SHIFT			(PAGE_SHIFT + TABLE_SHIFT)

#define PAGE_SIZE   			(1 << PAGE_SHIFT)	
#define SECTION_SIZE			(1 << SECTION_SHIFT)	

#define LOW_MEMORY              	(2 * SECTION_SIZE)

#define TCR_VALUE  ( \
    (0b0LL   << 40) | /* [40]    HD: HW Dirty Bit Management - 0 = Disabled (0 = SW-managed, 1 = HW-managed) */ \
    (0b0LL   << 39) | /* [39]    HA: HW Access Flag - 0 = Disabled (0 = SW-managed, 1 = HW-managed) */ \
    (0b0LL   << 38) | /* [38]    TBI1: Top Byte Ignore (TTBR1) - 0 = Disabled (0 = Disabled, 1 = Enabled) */ \
    (0b0LL   << 37) | /* [37]    TBI0: Top Byte Ignore (TTBR0) - 0 = Disabled (0 = Disabled, 1 = Enabled) */ \
    (0b1LL   << 36) | /* [36]    AS: Address Space ID - 1 = 16-bit ASID (0 = 8-bit, 1 = 16-bit) */ \
    (0b0LL   << 35) | /* [35]    Reserved - Must be 0 */ \
    (0b000LL << 32) | /* [34:32] IPS: PA Size - 000 = 4GB (000 = 4GB, 001 = 64GB, 010 = 1TB) */ \
    (0b10LL  << 30) | /* [31:30] TG1: Granule (TTBR1) - 10 = 4KB (00 = 16KB, 01 = 64KB, 10 = 4KB) */ \
    (0b10LL  << 28) | /* [29:28] SH1: Shareability (TTBR1) - 10 = Outer Shareable (00 = Non-shareable, 10 = Outer, 11 = Inner) */ \
    (0b00LL  << 26) | /* [27:26] ORGN1: Outer Cache (TTBR1) - 00 = Non-cacheable (00 = NC, 01 = WB, 10 = WT, 11 = WB Non-NC) */ \
    (0b00LL  << 24) | /* [25:24] IRGN1: Inner Cache (TTBR1) - 00 = Non-cacheable (Same options as ORGN1) */ \
    (0b0LL   << 23) | /* [23]    EPD1: TT Walk (TTBR1) - 0 = Enabled (0 = Walk, 1 = Fault) */ \
    (0b0LL   << 22) | /* [22]    A1: ASID Selection - 0 = Uses TTBR0 ASID (0 = TTBR0, 1 = TTBR1) */ \
    (16LL    << 16) | /* [21:16] T1SZ: VA Size (TTBR1) - 16 = 256TB (16 = 256TB, 25 = 512GB, 32 = 4GB) */ \
    (0b0LL   << 15) | /* [15]    Reserved - Must be 0 */ \
    (0b00LL  << 14) | /* [15:14] TG0: Granule (TTBR0) - 00 = 4KB (00 = 4KB, 01 = 64KB, 10 = 16KB) */ \
    (0b10LL  << 12) | /* [13:12] SH0: Shareability (TTBR0) - 10 = Outer Shareable (00 = Non-shareable, 10 = Outer, 11 = Inner) */ \
    (0b00LL  << 10) | /* [11:10] ORGN0: Outer Cache (TTBR0) - 00 = Non-cacheable (00 = NC, 01 = WB, 10 = WT, 11 = WB Non-NC) */ \
    (0b00LL  << 8)  | /* [9:8]   IRGN0: Inner Cache (TTBR0) - 00 = Non-cacheable (Same options as ORGN0) */ \
    (0b0LL   << 7)  | /* [7]     EPD0: TT Walk (TTBR0) - 0 = Enabled (0 = Walk, 1 = Fault) */ \
    (0b0LL   << 6)  | /* [6]     Reserved - Must be 0 */ \
    (16LL    << 0)    /* [5:0]   T0SZ: VA Size (TTBR0) - 16 = 256TB (Same options as T1SZ) */ \
)





#define MAIR_VALUE 0x0000000000004400
// #define SCTLR_MMU_ENABLED 0x1

#ifndef __ASSEMBLER__

void memzero(unsigned long src, unsigned long n);
extern "C" void create_page_tables();
extern "C" void init_mmu();
#endif

#endif  /*_MM_H */
