#include "elf_loader.h"
#include "printf.h"
#include "mm.h"
#include "heap.h"

#define ERROR(msg...) printf(msg);

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
		return false;
	}
	if(hdr->e_ident[EI_VERSION] != EV_CURRENT) {
		ERROR("Unsupported ELF File version.\n");
		return false;
	}
	if(hdr->e_type != ET_REL && hdr->e_type != ET_EXEC) {
		ERROR("Unsupported ELF File type.\n");
		return false;
	}
	return true;
}

void *elf_load(void* ptr) {
	Elf64_Ehdr *hdr = (Elf64_Ehdr *)ptr;
	if(!elf_check_supported(hdr)) {
		ERROR("ELF File cannot be loaded.\n");
		return nullptr;
	}
	switch(hdr->e_type) {
		case ET_EXEC:
			// TODO : Implement
			return nullptr;
		case ET_REL:
			return elf_load(hdr);
	}
	return nullptr;
} 

# define ELF_RELOC_ERR -1

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

		extern void *elf_lookup_symbol(const char *name);
		void *target = elf_lookup_symbol(name);

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

static int elf_load_stage1(Elf64_Ehdr *hdr) {
	Elf64_Shdr *shdr = elf_sheader(hdr);

	unsigned int i;
	// Iterate over section headers
	for(i = 0; i < hdr->e_shnum; i++) {
		Elf64_Shdr *section = &shdr[i];

		// If the section isn't present in the file
		if(section->sh_type == SHT_NOBITS) {
			// Skip if it the section is empty
			if(!section->sh_size) continue;
			// If the section should appear in memory
			if(section->sh_flags & SHF_ALLOC) {
				// Allocate and zero some memory
				void *mem = malloc(section->sh_size);
				memset(mem, 0, section->sh_size);
				// Assign the memory offset to the section offset
				section->sh_offset = (uint64_t)mem - (uint64_t)hdr;
			}
		}
		// TODO parse the other shtypes
	}
	return 0;
}

// relocation

# define DO_ARM_32(S, A)	((S) + (A))
# define DO_ARM_PC32(S, A, P)	((S) + (A) - (P))

static uint64_t elf_do_reloc(Elf64_Ehdr *hdr, Elf64_Rel *rel, Elf64_Shdr *reltab) {
	Elf64_Shdr *target = elf_section(hdr, reltab->sh_info);

	uint64_t addr = (uint64_t)hdr + target->sh_offset;
	uint64_t *ref = (uint64_t *)(addr + rel->r_offset);
	// Symbol value
	uint64_t symval = 0;
	if(ELF64_R_SYM(rel->r_info) != SHN_UNDEF) {
		symval = elf_get_symval(hdr, reltab->sh_link, ELF64_R_SYM(rel->r_info));
		if(symval == ELF_RELOC_ERR) return ELF_RELOC_ERR;
	}
	// Relocate based on type
	//
	// LIKELY WRONG: LOOK INTO ARM RELOCATION TYPES LATER
	//
	//
	//
	switch(ELF64_R_TYPE(rel->r_info)) {
		case R_ARM64_NONE:
			// No relocation
			break;
		case R_ARM64_32:
			// Symbol + Offset
			*ref = DO_ARM_32(symval, *ref);
			break;
		case R_ARM64_PC32:
			// Symbol + Offset - Section Offset
			*ref = DO_ARM_PC32(symval, *ref, (uint64_t)ref);
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
			for(idx = 0; idx < section->sh_size / section->sh_entsize; idx++) {
				Elf64_Rel *reltab = &((Elf64_Rel *)((uint64_t)hdr + section->sh_offset))[idx];
				int result = elf_do_reloc(hdr, reltab, section);
				// On error, display a message and return
				if(result == ELF_RELOC_ERR) {
					ERROR("Failed to relocate symbol.\n");
					return ELF_RELOC_ERR;
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
        ERROR("mmap failed");
        return nullptr;
    }

    // Read segment content from memory
	memcpy((void*)((uint64_t)mapped_memory + page_offset), (void*)((uint64_t)mem + offset), file_size)

    // Zero out the remaining part of the memory (bss segment)
    if (mem_size > file_size) {
        memset(mapped_memory + page_offset + file_size, 0, mem_size - file_size);
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
