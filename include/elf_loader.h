#ifndef _EVENT_H
#define _EVENT_H
#include "stdint.h"

typedef uint16_t Elf64_Half;	// Unsigned half int
typedef uint64_t Elf64_Off;	// Unsigned offset
typedef uint64_t Elf64_Addr;	// Unsigned address
typedef uint32_t Elf64_Word;	// Unsigned int
typedef int32_t  Elf64_Sword;	// Signed int
typedef uint64_t Elf64_Xword; // long
typedef int64_t	 Elf64_Sxword; // signed long

# define EI_NIDENT	16

typedef struct {
	unsigned char   e_ident[EI_NIDENT];
	Elf64_Half      e_type;
	Elf64_Half      e_machine;
	Elf64_Word      e_version;
	Elf64_Addr      e_entry;
	Elf64_Off       e_phoff;
	Elf64_Off       e_shoff;
	Elf64_Word      e_flags;
	Elf64_Half      e_ehsize;
	Elf64_Half      e_phentsize;
	Elf64_Half      e_phnum;
	Elf64_Half      e_shentsize;
	Elf64_Half      e_shnum;
	Elf64_Half      e_shstrndx;
} Elf64_Ehdr;

enum Elf_Ident {
	EI_MAG0		= 0, // 0x7F
	EI_MAG1		= 1, // 'E'
	EI_MAG2		= 2, // 'L'
	EI_MAG3		= 3, // 'F'
	EI_CLASS	= 4, // Architecture (64/64)
	EI_DATA		= 5, // Byte Order
	EI_VERSION	= 6, // ELF Version
	EI_OSABI	= 7, // OS Specific
	EI_ABIVERSION	= 8, // OS Specific
	EI_PAD		= 9  // Padding
};

enum Elf_Type {
	ET_NONE		= 0, // Unknown Type
	ET_REL		= 1, // Relocatable File
	ET_EXEC		= 2, // Executable File
    ET_DYN      = 3, // Shared Object
    ET_CORE     = 4, // Core File
};



# define ELFMAG0	0x7F // e_ident[EI_MAG0]
# define ELFMAG1	'E'  // e_ident[EI_MAG1]
# define ELFMAG2	'L'  // e_ident[EI_MAG2]
# define ELFMAG3	'F'  // e_ident[EI_MAG3]
# define ELFCLASS64	2  // 64-bit Architecture
# define EM_ARM	    0xB7
# define EV_CURRENT 1
# define ELFDATA2LSB	1  // Little Endian

// symbol table

# define SHN_UNDEF	(0x00) // Undefined/Not Present
# define SHN_ABS    0xFFF1 // Absolute symbol

typedef struct {
	Elf64_Word	st_name;
	unsigned char	st_info;
	unsigned char	st_other;
	Elf64_Half	st_shndx;
	Elf64_Addr	st_value;
	Elf64_Xword	st_size;
} Elf64_Sym;


# define ELF64_ST_BIND(INFO)	((INFO) >> 4)
# define ELF64_ST_TYPE(INFO)	((INFO) & 0x0F)

enum StT_Bindings {
	STB_LOCAL		= 0, // Local scope
	STB_GLOBAL		= 1, // Global scope
	STB_WEAK		= 2  // Weak, (ie. __attribute__((weak)))
};

enum StT_Types {
	SHT_NULL		= 0,
	SHT_PROGBITS	= 1, 
	SHT_SYMTAB		= 2,
	SHT_STRTAB		= 3,
	SHT_RELA		= 4, 
	SHT_HASH		= 5, 
	SHT_DYNAMIC		= 6, 
	SHT_NOTE		= 7, 
	SHT_NOBITS		= 8, 
	SHT_REL			= 9,   
	SHT_SHLIB		= 10, 
	SHT_DYNSYM		= 11, 
	SHT_INIT_ARRAY	= 12, 
	SHT_FINI_ARRAY	= 13,   
	SHT_PREINIT_ARRAY		= 14, 
	SHT_GROUP		= 15, 
	SHT_SYMTAB_SHNDX	= 16,   
};

enum ShT_Attributes {
	SHF_WRITE	= 0x01, // Writable section
	SHF_ALLOC	= 0x02,  // Exists in memory
	SHF_EXECINSTR = 0x03,
};

enum StT_Visability {
	STV_DEFAULT 	= 0, // global and weak symbols are visible outside of their defining component
	STV_PROTECTED	= 1, // protected if it is visible in other components but not preemptable
	STV_HIDDEN		= 2, // name not visible to other components
	STV_INTERNAL	= 3  // whatever you want
};

// relocations

typedef struct {
	Elf64_Addr	r_offset;
	Elf64_Xword	r_info;
} Elf64_Rel;

typedef struct {
	Elf64_Addr	r_offset;
	Elf64_Xword	r_info;
	Elf64_Sxword	r_addend;
} Elf64_Rela;

#define ELF64_R_SYM(i)    ((i)>>32)
#define ELF64_R_TYPE(i)   ((i)&0xffffffffL)
#define ELF64_R_INFO(s,t) (((s)<<32)+((t)&0xffffffffL))

enum RtT_Types {
	R_ARM_NONE=0,
	R_AARCH64_NONE=256,
	/* Static Data relocations */
	R_AARCH64_ABS64=257,
	R_AARCH64_ABS32=258,
	R_AARCH64_ABS16=259,
	R_AARCH64_PREL64=260,
	R_AARCH64_PREL32=261,
	R_AARCH64_PREL16=262,
	R_AARCH64_MOVW_UABS_G0=263,
	R_AARCH64_MOVW_UABS_G0_NC=264,
	R_AARCH64_MOVW_UABS_G1=265,
	R_AARCH64_MOVW_UABS_G1_NC=266,
	R_AARCH64_MOVW_UABS_G2=267,
	R_AARCH64_MOVW_UABS_G2_NC=268,
	R_AARCH64_MOVW_UABS_G3=269,
	R_AARCH64_MOVW_SABS_G0=270,
	R_AARCH64_MOVW_SABS_G1=271,
	R_AARCH64_MOVW_SABS_G2=272,
	/* PC-relative addresses */
	R_AARCH64_LD_PREL_LO19=273,
	R_AARCH64_ADR_PREL_LO21=274,
	R_AARCH64_ADR_PREL_PG_HI21=275,
	R_AARCH64_ADR_PREL_PG_HI21_NC=276,
	R_AARCH64_ADD_ABS_LO12_NC=277,
	R_AARCH64_LDST8_ABS_LO12_NC=278,
	/* Control-flow relocations */
	R_AARCH64_TSTBR14=279,
	R_AARCH64_CONDBR19=280,
	R_AARCH64_JUMP26=282,
	R_AARCH64_CALL26=283,
	R_AARCH64_LDST16_ABS_LO12_NC=284,
	R_AARCH64_LDST32_ABS_LO12_NC=285,
	R_AARCH64_LDST64_ABS_LO12_NC=286,
	R_AARCH64_LDST128_ABS_LO12_NC=299,
	R_AARCH64_MOVW_PREL_G0=287,
	R_AARCH64_MOVW_PREL_G0_NC=288,
	R_AARCH64_MOVW_PREL_G1=289,
	R_AARCH64_MOVW_PREL_G1_NC=290,
	R_AARCH64_MOVW_PREL_G2=291,
	R_AARCH64_MOVW_PREL_G2_NC=292,
	R_AARCH64_MOVW_PREL_G3=293,
	/* Dynamic relocations */
	R_AARCH64_COPY=1024,
	R_AARCH64_GLOB_DAT=1025,   /* Create GOT entry.  */
	R_AARCH64_JUMP_SLOT=1026,   /* Create PLT entry.  */
	R_AARCH64_RELATIVE=1027,   /* Adjust by program base.  */
	R_AARCH64_TLS_TPREL64=1030,
	R_AARCH64_TLS_DTPREL32=1031,
};

typedef struct {
	Elf64_Word	sh_name;
	Elf64_Word	sh_type;
	Elf64_Xword	sh_flags;
	Elf64_Addr	sh_addr;
	Elf64_Off	sh_offset;
	Elf64_Xword	sh_size;
	Elf64_Word	sh_link;
	Elf64_Word	sh_info;
	Elf64_Xword	sh_addralign;
	Elf64_Xword	sh_entsize;
} Elf64_Shdr;

static inline Elf64_Shdr *elf_sheader(Elf64_Ehdr *hdr) {
	return (Elf64_Shdr *)((uint64_t)hdr + hdr->e_shoff);
}

static inline Elf64_Shdr *elf_section(Elf64_Ehdr *hdr, int idx) {
	return &elf_sheader(hdr)[idx];
}

typedef struct {
	Elf64_Word	p_type;
	Elf64_Word	p_flags;
	Elf64_Off	p_offset;
	Elf64_Addr	p_vaddr;
	Elf64_Addr	p_paddr;
	Elf64_Xword	p_filesz;
	Elf64_Xword	p_memsz;
	Elf64_Xword	p_align;
} Elf64_Phdr;

enum Program_Type {
	PT_NULL = 0, // null
	PT_LOAD = 1, // loadable
	PT_DYNAMIC = 2, // for dynamic linking
	PT_INTERP = 3, // invoke a path name for an interpreter (MUST BE NULL TERMINATED)
	PT_NOTE = 4, // location and size of auxillary info
	PT_SHLIB = 5, 
	PT_PHDR = 6, // specifies size and location of program header
	PT_TLS = 7 // thread local storage
};

enum Program_Flags {
	PF_X = 1, // can execute
	PF_W = 2, // can write
	PF_R = 4, // can read
};

static inline Elf64_Phdr *elf_pheader(Elf64_Ehdr *hdr) {
	return (Elf64_Phdr *)((uint64_t)hdr + hdr->e_phoff);
}

static inline Elf64_Phdr *elf_program(Elf64_Ehdr *hdr, int idx) {
	return &elf_pheader(hdr)[idx];
}

void* elf_load(void* ptr);

#endif