#include "elf_loader.h"
#include "printf.h"
#include "mm.h"
#include "heap.h"
#include "libk.h"

#define ERROR(msg...) printf(msg);

#define MAP_PRIVATE 1
#define MAP_ANONYMOUS 2
#define MAP_FAILED (void*)-1
#define PROT_EXEC 1
#define PROT_READ 2
#define PROT_WRITE 4
#define PROT_NONE 0
void *mmap(void* addr, size_t length, int prot, int flags, int fd, int offset) {
	printf("calling mmap at %x, with size %d, prot %x, flags %x, fd %d, and offset %d\n", addr, length, prot, flags, fd, offset);
	return MAP_FAILED;
}

bool elf_check_file(Elf64_Ehdr *hdr) {
    if (!hdr) return false;
	if(hdr->e_ident[EI_MAG0] != ELFMAG0) {
		ERROR("ELF Header EI_MAG0 incorrect.\n");
		return false;
	}
	if(hdr->e_ident[EI_MAG1] != ELFMAG1) {
		ERROR("ELF Header EI_MAG1 incorrect.\n");
		return false;
	}
	if(hdr->e_ident[EI_MAG2] != ELFMAG2) {
		ERROR("ELF Header EI_MAG2 incorrect.\n");
		return false;
	}
	if(hdr->e_ident[EI_MAG3] != ELFMAG3) {
		ERROR("ELF Header EI_MAG3 incorrect.\n");
		return false;
	}
	return true;
}

bool elf_check_supported(Elf64_Ehdr *hdr) {
	if(!elf_check_file(hdr)) {
		ERROR("Invalid ELF File.\n");
		return false;
	}
	if(hdr->e_ident[EI_CLASS] != ELFCLASS64) {
		ERROR("Unsupported ELF File Class.\n");
		return false;
	}
	if(hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		ERROR("Unsupported ELF File byte order.\n");
		return false;
	}
	if(hdr->e_machine != EM_ARM) {
		ERROR("Unsupported ELF File target.\n");
		//return false;
	}
	if(hdr->e_ident[EI_VERSION] != EV_CURRENT) {
		ERROR("Unsupported ELF File version.\n");
		return false;
	}
	if(hdr->e_type != ET_REL && hdr->e_type != ET_EXEC && hdr->e_type != ET_DYN) {
		ERROR("Unsupported ELF File type. %d\n", hdr->e_type);
		return false;
	}
	return true;
}

# define ELF_RELOC_ERR -1

Elf64_Shdr *find_section(Elf64_Ehdr* hdr, const char* name) {
	Elf64_Shdr *shdr = elf_sheader(hdr);
	for(int i = 0; i < hdr->e_shnum; i++) {
		
		Elf64_Shdr *section = &shdr[i];
		Elf64_Shdr *shstrndx = elf_section(hdr, hdr->e_shstrndx);
		const char *cname = (const char *)hdr + shstrndx->sh_offset + section->sh_name;
		if (K::strcmp(name, cname) == 0) {
			return section;
		}
	}
	return nullptr;
}

void* elf_lookup_symbol (Elf64_Ehdr* hdr, const char* name) { 
	Elf64_Shdr *dynsym = find_section(hdr, ".dynsym");
	Elf64_Shdr *dynstr = find_section(hdr, ".dynstr");

	if (!dynsym || !dynstr) {
		ERROR("Symbol or string table not found!");
		return nullptr;
	}

	uint64_t dynsym_entries = dynsym->sh_size / dynsym->sh_entsize;
	uint64_t symaddr = (uint64_t)hdr + dynsym->sh_offset;
	for (int i = 0; i < dynsym_entries; i++) {
		Elf64_Sym *symbol = &((Elf64_Sym *)symaddr)[i];
		const char *cname = (const char*)((uint64_t)hdr + dynstr->sh_offset + symbol->st_name);
		if (K::strcmp(name, cname) == 0) {
			printf("WE FOUND %s, returning %x\n", cname, symbol->st_value);
			return (void*)symbol->st_value;
		}
	}
	return nullptr;
}

static uint64_t elf_get_symval(Elf64_Ehdr *hdr, int table, uint64_t idx) {
	if(table == SHN_UNDEF || idx == SHN_UNDEF) return 0;
	Elf64_Shdr *symtab = elf_section(hdr, table);

	uint64_t symtab_entries = symtab->sh_size / symtab->sh_entsize;
	if(idx >= symtab_entries) {
		ERROR("Symbol Index out of Range (%d:%u).\n", table, idx);
		return ELF_RELOC_ERR;
	}

	uint64_t symaddr = (uint64_t)hdr + symtab->sh_offset;
	Elf64_Sym *symbol = &((Elf64_Sym *)symaddr)[idx];
	if(symbol->st_shndx == SHN_UNDEF) {
		// External symbol, lookup value
		Elf64_Shdr *strtab = elf_section(hdr, symtab->sh_link);
		const char *name = (const char *)hdr + strtab->sh_offset + symbol->st_name;

		void *target = elf_lookup_symbol(hdr, name);

		if(target == nullptr) {
			// Extern symbol not found
			if(ELF64_ST_BIND(symbol->st_info) & STB_WEAK) {
				// Weak symbol initialized as 0
				return 0;
			} else {
				ERROR("Undefined External Symbol : %s.\n", name);
				return ELF_RELOC_ERR;
			}
		} else {
			return (uint64_t)target;
		}
	} else if(symbol->st_shndx == SHN_ABS) {
		// Absolute symbol
		return symbol->st_value;
	} else {
		// Internally defined symbol
		Elf64_Shdr *target = elf_section(hdr, symbol->st_shndx);
		return (uint64_t)hdr + symbol->st_value + target->sh_offset;
	}
}

// create bss sections (sections with a bunch of zeroes)

uint64_t copy_memory(void* start, Elf64_Xword flags, Elf64_Xword size, Elf64_Ehdr *hdr) {
	if (flags & SHF_ALLOC) {
		void* mem = kmalloc(size);
		K::memcpy(mem, start, size);
		if (flags & SHF_WRITE) {
			// make it writable
		}
		if (flags & SHF_EXECINSTR) {
			// make it executable
		}
		return (uint64_t)mem - (uint64_t)hdr;
	}
	return nullptr;
}

static int elf_load_stage1(Elf64_Ehdr *hdr) {
	Elf64_Shdr *shdr = elf_sheader(hdr);

	unsigned int i;
	// Iterate over section headers
	for(i = 0; i < hdr->e_shnum; i++) {
		Elf64_Shdr *section = &shdr[i];
		switch (section->sh_type) {
			case SHT_NULL:
				break;
			case SHT_PROGBITS:
				section->sh_offset = copy_memory((void*)section->sh_addr, section->sh_flags, section->sh_size, hdr);
				break;
			case SHT_SYMTAB:
				// LINKING STUFF
				break;
			case SHT_RELA: 
				// not here
				break;
			case SHT_HASH: 
				section->sh_offset = copy_memory((void*)section->sh_addr, section->sh_flags, section->sh_size, hdr);
				break;
			case SHT_DYNAMIC: 
				section->sh_offset = copy_memory((void*)section->sh_addr, section->sh_flags, section->sh_size, hdr);
				break;
			case SHT_NOTE: 
				section->sh_offset = copy_memory((void*)section->sh_addr, section->sh_flags, section->sh_size, hdr);
				break;
			case SHT_NOBITS: 
				// Skip if it the section is empty
				if(!section->sh_size) continue;
				// If the section should appear in memory
				if(section->sh_flags & SHF_ALLOC) {
					// Allocate and zero some memory
					void *mem = kmalloc(section->sh_size);
					// memset(mem, 0, section->sh_size);
					// Assign the memory offset to the section offset
					section->sh_offset = (uint64_t)mem - (uint64_t)hdr;
				}
				break;
			case SHT_REL: 
				section->sh_offset = copy_memory((void*)section->sh_addr, section->sh_flags, section->sh_size, hdr);
				break;
			case SHT_SHLIB: 
				section->sh_offset = copy_memory((void*)section->sh_addr, section->sh_flags, section->sh_size, hdr);
				break;
			case SHT_STRTAB:
			case SHT_DYNSYM: 
				section->sh_offset = copy_memory((void*)section->sh_addr, section->sh_flags, section->sh_size, hdr);
				break;
			case SHT_INIT_ARRAY: 
				section->sh_offset = copy_memory((void*)section->sh_addr, section->sh_flags, section->sh_size, hdr);
				break;
			case SHT_FINI_ARRAY: 
				section->sh_offset = copy_memory((void*)section->sh_addr, section->sh_flags, section->sh_size, hdr);
				break;
			case SHT_GROUP: 
				// do something
				break;
			case SHT_SYMTAB_SHNDX: 
				// do something 
				break;
			case SHT_PREINIT_ARRAY:
			case SHT_ARM_ATTRIBUTES:
			case SHT_ARM_DEBUGOVERLAY:
			case SHT_ARM_OVERLAYSECTION:
			case SHT_SunwSignature:
			case SHT_SunwAnnotate:
			case SHT_SunwDebugstr:
			case SHT_SunwDebug:
			case SHT_SunwMove:
			case SHT_SunwComdat:
			case SHT_SunwSyminfo:
			case SHT_SunwVerdef:
			case SHT_SunwVerneed:
			case SHT_SunwVersym:
				// skipping - not sure if necessary
				break;
			default:
				ERROR("undefined shtype: %d\n", section->sh_type);
		}
	}
	return 0;
}

// relocation
static uint64_t elf_do_reloc(Elf64_Ehdr *hdr, Elf64_Rela *rel, Elf64_Shdr *reltab) {
	Elf64_Shdr *target = elf_section(hdr, reltab->sh_info);

	uint64_t addr = (uint64_t)hdr + target->sh_offset;
	uint64_t *ref = (uint64_t *)(addr + rel->r_offset);
	// Symbol value
	uint64_t symval = 0;
	if(ELF64_R_SYM(rel->r_info) != SHN_UNDEF) {
		symval = elf_get_symval(hdr, reltab->sh_link, ELF64_R_SYM(rel->r_info));
		if(symval == (uint8_t)ELF_RELOC_ERR) return ELF_RELOC_ERR;
	}
	// Relocate based on type
	//
	printf("THE RELOCATIONTYPE IS %d\n", ELF64_R_TYPE(rel->r_info));
	switch(ELF64_R_TYPE(rel->r_info)) {
		case R_AARCH64_ABS64:
		case R_AARCH64_GLOB_DAT:
		case R_AARCH64_JUMP_SLOT:
			*ref = rel->r_addend;
			break;
		case R_AARCH64_ABS32:
			*ref = (*ref + rel->r_addend) & 0xFFFFFFFF;
			break;
		case R_AARCH64_ABS16:
			*ref = (*ref + rel->r_addend) & 0xFFFF;
			break;
		case R_AARCH64_PREL64:
			*ref += rel->r_addend - rel->r_offset;
			break;
		case R_AARCH64_PREL32:
			*ref = (*ref + rel->r_addend - rel->r_offset) & 0xFFFFFFFF;
			break;
		case R_AARCH64_PREL16:
			*ref = (*ref + rel->r_addend - rel->r_offset) & 0xFFFF;
			break;
		case R_AARCH64_MOVW_UABS_G0:
			*ref = rel->r_addend & 0xFFFF;
			break;
		case R_AARCH64_MOVW_UABS_G1:
			*ref = (rel->r_addend >> 16) & 0xFFFF;
			break;
		case R_AARCH64_MOVW_UABS_G2:
			*ref = (rel->r_addend >> 32) & 0xFFFF;
			break;
		case R_AARCH64_MOVW_UABS_G3:
			*ref = (rel->r_addend >> 48) & 0xFFFF;
			break;
		case R_AARCH64_RELATIVE:
			*ref = *ref + rel->r_addend;
			break;
		case R_AARCH64_TLS_TPREL64:
			*ref = rel->r_addend;
			break;
		default:
			// Relocation type not supported, display error and return
			ERROR("Unsupported Relocation Type (%d).\n", ELF64_R_TYPE(rel->r_info));
			return ELF_RELOC_ERR;
	}
	return symval;
}

# define ELF_PHDR_ERR -2

static int elf_load_stage2(Elf64_Ehdr *hdr) {
	Elf64_Shdr *shdr = elf_sheader(hdr);

	unsigned int i, idx;
	// Iterate over section headers
	for(i = 0; i < hdr->e_shnum; i++) {
		Elf64_Shdr *section = &shdr[i];
		
		// If this is a relocation section
		if(section->sh_type == SHT_REL) {
			// Process each entry in the table
			/*for(idx = 0; idx < section->sh_size / section->sh_entsize; idx++) {
				Elf64_Rel *reltab = &((Elf64_Rel *)((uint64_t)hdr + section->sh_offset))[idx];
				int result = elf_do_reloc(hdr, reltab, section);
				// On error, display a message and return
				if(result == ELF_RELOC_ERR) {
					ERROR("Failed to relocate symbol.\n");
					return ELF_RELOC_ERR;
				}
			}*/
			ERROR("ITS SHT_REL...");
			continue;
		}
		
		if(section->sh_type == SHT_RELA) {
			printf("THIS IS A SHTRELA %d!\n", i);
			// Process each entry in the table
			for(idx = 0; idx < section->sh_size / section->sh_entsize; idx++) {
				Elf64_Rela *reltab = &((Elf64_Rela *)((uint64_t)hdr + section->sh_offset))[idx];
				printf("off: %x info: %x addend: %x\n", reltab->r_offset, reltab->r_info, reltab->r_addend);
				int result = elf_do_reloc(hdr, reltab, section);
				// On error, display a message and return
				if(result == ELF_RELOC_ERR) {
					ERROR("Failed to relocate symbol.\n");
					//return ELF_RELOC_ERR;
				}
			}
		}
		// TODO: figure out what the hell other shtypes do
	}
	return 0;
}

void *load_segment_mem(void* mem, Elf64_Phdr *phdr) {
    size_t mem_size = phdr->p_memsz;
    size_t file_size = phdr->p_filesz;
    off_t offset = phdr->p_offset;
    void *vaddr = (void *)(phdr->p_vaddr);
    
    // Ensure page alignment for mmap
    size_t page_size = PAGE_SIZE;
    off_t page_offset = offset % page_size;
    off_t aligned_offset = offset - page_offset;
    size_t aligned_size = mem_size + page_offset;

    // mmap the memory region with the correct protections
    int prot = 0;
    if (phdr->p_flags & PF_R) prot |= PROT_READ;
    if (phdr->p_flags & PF_W) prot |= PROT_WRITE;
    if (phdr->p_flags & PF_X) prot |= PROT_EXEC;

    void *mapped_memory = mmap(vaddr, aligned_size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapped_memory == MAP_FAILED) {
        ERROR("mmap failed\n");
        return nullptr;
    }

    // Read segment content from memory
	K::memcpy((void*)((uint64_t)mapped_memory + page_offset), (void*)((uint64_t)mem + offset), file_size);

    // Zero out the remaining part of the memory (bss segment)
    if (mem_size > file_size) {
        //K::memset(mapped_memory + page_offset + file_size, 0, mem_size - file_size);
    }

    return mapped_memory;
}

static int elf_load_stage3(Elf64_Ehdr *hdr) {
	Elf64_Phdr *phdr = elf_pheader(hdr);
	unsigned int i, idx;
	for (i = 0; i < hdr->e_phnum; i++) {
		Elf64_Phdr *prog = &phdr[i];
		switch(prog->p_type) {
			case PT_NULL:
				break;
			case PT_LOAD:
				// load this in
				load_segment_mem((void*) hdr, prog);
				break;
			case PT_DYNAMIC:
				// DYNAMIC LINKING GOES HERE
				break;
			case PT_INTERP: 
				// The array element specifies the location and size of a nullptr-terminated path name 
				// to invoke as an interpreter. This segment type is meaningful only for executable 
				// files (though it may occur for shared objects); it may not occur more than once
				// in a file. If it is present, it must precede any loadable segment entry. 
				// See ``Program Interpreter'' below for more information.
				// https://refspecs.linuxfoundation.org/elf/gabi4+/ch5.dynamic.html#interpreter
				break;
			case PT_NOTE:
				// https://refspecs.linuxfoundation.org/elf/gabi4+/ch5.pheader.html#note_section
				break;
			case PT_TLS: 
				// https://refspecs.linuxfoundation.org/elf/gabi4+/ch5.pheader.html#tls
				// unnecessary..?
				break;
			case PT_PHDR: 
				// load this in
				load_segment_mem((void*) hdr, prog);
				break;
			default:
				ERROR("unsupported program header type.\n");
		}
	}
	return 0;
}

// load
static inline void *elf_load_rel(Elf64_Ehdr *hdr) {
	int result;
	result = elf_load_stage1(hdr);
	if(result == ELF_RELOC_ERR) {
		ERROR("Unable to load ELF file.\n");
		return nullptr;
	}
	result = elf_load_stage2(hdr);
	if(result == ELF_RELOC_ERR) {
		ERROR("Unable to load ELF file.\n");
		return nullptr;
	}
	// Parse the program header (if present)
	result = elf_load_stage3(hdr);
	if (result == ELF_PHDR_ERR) {
		ERROR("Unable to load ELF file.\n");
		return nullptr;
	}
	return (void *)hdr->e_entry;
}

// load
static inline void *elf_load_exec(Elf64_Ehdr *hdr) {
	printf("we are hjere now\n");
	int result;
	result = elf_load_stage1(hdr);
	if(result == ELF_RELOC_ERR) {
		ERROR("Unable to load ELF file.\n");
		return nullptr;
	}
	// Parse the program header (if present)
	result = elf_load_stage3(hdr);
	if (result == ELF_PHDR_ERR) {
		ERROR("Unable to load ELF file.\n");
		return nullptr;
	}
	return (void *)hdr->e_entry;
}

void *elf_load(void* ptr) {
	Elf64_Ehdr *hdr = (Elf64_Ehdr *)ptr;
	if(!elf_check_supported(hdr)) {
		ERROR("ELF File cannot be loaded.\n");
		return nullptr;
	}
	printf("%d type\n", hdr->e_type);
	switch(hdr->e_type) {
		case ET_EXEC:
			return elf_load_exec(hdr);
		case ET_REL:
			return elf_load_rel(hdr);
		case ET_DYN:
			return elf_load_rel(hdr);
			// todo ... ?? ? ? ? ? ??
			return nullptr;
	}
	return nullptr;
} 
