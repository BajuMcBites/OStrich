#ifndef	_MM_H
#define	_MM_H

#define VA_START 			0xffff000000000000

#define PAGE_SHIFT	 		12
#define TABLE_SHIFT 			9
#define SECTION_SHIFT			(PAGE_SHIFT + TABLE_SHIFT)

#define PAGE_SIZE   			(1 << PAGE_SHIFT)	
#define SECTION_SIZE			(1 << SECTION_SHIFT)	

#define LOW_MEMORY              	(2 * SECTION_SIZE)

#define TCR_VALUE 0x850102010 
#define MAIR_VALUE 0x0000000000004400
// #define SCTLR_MMU_ENABLED 0x1

#ifndef __ASSEMBLER__

void memzero(unsigned long src, unsigned long n);
extern "C" void create_page_tables();
extern "C" void init_mmu();
#endif

#endif  /*_MM_H */
