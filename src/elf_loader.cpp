#include "elf_loader.h"
#include "printf.h"
#include "mm.h"
#include "heap.h"
#include "libk.h"
#include "ramfs.h"
#include "event.h"
#include "mmap.h"

#define ERROR(msg...) printf(msg);

#define MAP_FAILED (void*)-1

LoadedLibrary *g_loaded_libs = nullptr;

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

Elf64_Shdr *find_section(Elf64_Ehdr* hdr, char* name) {
	Elf64_Shdr *shdr = elf_sheader(hdr);
	for(int i = 0; i < hdr->e_shnum; i++) {
		
		Elf64_Shdr *section = &shdr[i];
		Elf64_Shdr *shstrndx = elf_section(hdr, hdr->e_shstrndx);
		char *cname = (char *)hdr + shstrndx->sh_offset + section->sh_name;
		if (K::strcmp(name, cname) == 0) {
			return section;
		}
	}
	return nullptr;
}


void* elf_lookup_symbol_internal (Elf64_Ehdr* hdr, const char* name) { 
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
		char *cname = (char*)((uint64_t)hdr + dynstr->sh_offset + symbol->st_name);
		if (K::strcmp(name, cname) == 0) {
			printf("WE FOUND %s, returning %x\n", cname, symbol->st_value);
			return (void*)symbol->st_value;
		}
	}
	return nullptr;
}

void* elf_lookup_symbol(Elf64_Ehdr* hdr, const char* name) {
    // 1. Search the current ELF's dynamic symbols
    void *sym = elf_lookup_symbol_internal(hdr, name);
    if (sym) return sym;

    // 2. Search all loaded libraries
    for (LoadedLibrary *lib = g_loaded_libs; lib; lib = lib->next) {
        sym = elf_lookup_symbol_internal(lib->ehdr, name);
        if (sym) return sym;
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

void *load_section(void* mem, Elf64_Shdr *shdr, UserTCB* tcb) {
    size_t sect_size = shdr->sh_size;
    off_t sect_offset = shdr->sh_offset;
    void *vaddr = (void *)(shdr->sh_addr);
    
    // Ensure page alignment for mmap
    off_t page_offset = (uint64_t)vaddr % PAGE_SIZE;
    void* aligned_vaddr = (void*)((uint64_t)vaddr - page_offset);
    size_t aligned_size = sect_size + page_offset;

    // mmap the memory region with the correct protections
    int prot = 0;
    if (shdr->sh_flags & SHF_WRITE) prot |= PROT_WRITE;
    if (shdr->sh_flags & SHF_EXECINSTR) prot |= PROT_EXEC;
	printf("section vaddr: %x, size: %x\n", (uint64_t)aligned_vaddr, aligned_size);
	void* to = (void*)((uint64_t)aligned_vaddr + page_offset);
	void* from = (void*)(sect_offset);
	void* end = (void*)((uint64_t)from + sect_size);
	printf("section mapping 0x%x, from memory 0x%x\n", to, from);
	
    mmap(tcb, (uint64_t)aligned_vaddr, prot, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, nullptr, 0, aligned_size, 
		[=]() {
			load_mmapped_page(tcb, (uint64_t)aligned_vaddr, [=](uint64_t kvaddr) {
				tcb->page_table->use_page_table();
				void* page_to = (void*)kvaddr;
				if (page_to < to) {
					page_to = to;
				}
				void* page_from = ((uint64_t)page_to - (uint64_t)to) + mem;
				size_t cpy_size = PAGE_SIZE;
				if ((uint64_t)end - (uint64_t)page_from < cpy_size) {
					cpy_size = (size_t)((uint64_t)end - (uint64_t)page_from);
				}
				K::memcpy(page_to, page_from, cpy_size);
			});
		}
	);
    return (void*)vaddr;
}

static int elf_load_stage1(Elf64_Ehdr *hdr, UserTCB* tcb) {
	Elf64_Shdr *shdr = elf_sheader(hdr);

	unsigned int i;
	// Iterate over section headers
	for(i = 0; i < hdr->e_shnum; i++) {
		Elf64_Shdr *section = &shdr[i];
		switch (section->sh_type) {
			case SHT_NULL:
				break;
			case SHT_PROGBITS:
				load_section((void*)hdr, section, tcb);
				break;
			case SHT_SYMTAB:
				// LINKING STUFF
				break;
			case SHT_RELA: 
				// not here
				break;
			case SHT_HASH: 
				load_section((void*)hdr, section, tcb);
				break;
			case SHT_DYNAMIC: 
				load_section((void*)hdr, section, tcb);
				break;
			case SHT_NOTE: 
				load_section((void*)hdr, section, tcb);
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
				load_section((void*)hdr, section, tcb);
				break;
			case SHT_SHLIB: 
				load_section((void*)hdr, section, tcb);
				break;
			case SHT_STRTAB:
			case SHT_DYNSYM: 
				load_section((void*)hdr, section, tcb);
				break;
			case SHT_INIT_ARRAY: 
				load_section((void*)hdr, section, tcb);
				break;
			case SHT_FINI_ARRAY: 
				load_section((void*)hdr, section, tcb);
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

const char* elf_section_name(Elf64_Ehdr* hdr, Elf64_Shdr* shdr) {
    Elf64_Shdr *shstrtab = elf_section(hdr, hdr->e_shstrndx);
    if (!shstrtab) {
        ERROR("No .shstrtab section found!\n");
        return nullptr;
    }
    return (const char*)((uint64_t)hdr + shstrtab->sh_offset + shdr->sh_name);
}

static int elf_do_dynamic_reloc(Elf64_Ehdr *hdr, Elf64_Rela *rel, Elf64_Shdr *reltab) {
	// get section and symbol
	Elf64_Shdr *target = elf_section(hdr, reltab->sh_info);
    uint64_t addr = (uint64_t)hdr + target->sh_offset;
    uint64_t *ref = (uint64_t*)(addr + rel->r_offset);

	Elf64_Shdr *dynamic_sec = find_section(hdr, ".dynamic");
    if (!dynamic_sec) {
        ERROR("No .dynamic section!\n");
        return ELF_RELOC_ERR;
    }

	// try to use dynamic lookup with string table
	uint64_t dynstr_addr = 0;
    uint64_t dynsym_addr = 0;
    Elf64_Dyn *dyn = (Elf64_Dyn*)((uint64_t)hdr + dynamic_sec->sh_offset);
    
    for (; dyn->d_tag != DT_NULL; dyn++) {
        if (dyn->d_tag == DT_STRTAB) {
            dynstr_addr = dyn->d_un.d_ptr;
        } else if (dyn->d_tag == DT_SYMTAB) {
            dynsym_addr = dyn->d_un.d_ptr;
        }
    }

    if (!dynstr_addr || !dynsym_addr) {
        ERROR("Missing DT_STRTAB or DT_SYMTAB\n");
        return ELF_RELOC_ERR;
    }
	

	uint64_t sym_idx = ELF64_R_SYM(rel->r_info);
    Elf64_Sym *symbol = (Elf64_Sym*)(dynsym_addr + sym_idx * sizeof(Elf64_Sym));
    const char *symbol_name = (const char*)(dynstr_addr + symbol->st_name);

    uint64_t symval = (uint64_t)elf_lookup_symbol(hdr, symbol_name);
    if (!symval && !(ELF64_ST_BIND(symbol->st_info) & STB_WEAK)) {
        ERROR("Undefined symbol: %s\n", symbol_name);
        return ELF_RELOC_ERR;
    }


	// try to look up the symbol
    if (ELF64_R_SYM(rel->r_info) != SHN_UNDEF) {
        // from .dynsym
        Elf64_Shdr *dynsym = find_section(hdr, ".dynsym");
        Elf64_Shdr *dynstr = find_section(hdr, ".dynstr");
        if (!dynsym || !dynstr) {
            ERROR("Missing .dynsym or .dynstr\n");
            return ELF_RELOC_ERR;
        }

        Elf64_Sym *symbol = (Elf64_Sym*)((uint64_t)hdr + dynsym->sh_offset + 
            ELF64_R_SYM(rel->r_info) * sizeof(Elf64_Sym));
        symbol_name = (const char*)((uint64_t)hdr + dynstr->sh_offset + symbol->st_name);

        // global
        symval = (uint64_t)elf_lookup_symbol(hdr, symbol_name);
        if (symval == 0) {
            ERROR("Undefined symbol: %s\n", symbol_name);
            return ELF_RELOC_ERR;
        }
    }

    // Apply relocation
    switch (ELF64_R_TYPE(rel->r_info)) {
        case R_AARCH64_JUMP_SLOT: 
        case R_AARCH64_GLOB_DAT:   
            *ref = symval + rel->r_addend;
            break;

        case R_AARCH64_RELATIVE:
            *ref = (uint64_t)hdr + rel->r_addend;
            break;

		case R_AARCH64_ABS64:
            *ref = symval + rel->r_addend;
            break;

		// We got a tls offset?
        // case R_AARCH64_TLS_TPREL64:
        //     *ref = symval + rel->r_addend - TLS_OFFSET;
        //     break;

        default:
            ERROR("Unsupported dynamic relocation type: %d\n", ELF64_R_TYPE(rel->r_info));
            return ELF_RELOC_ERR;
    }

    return 0;
}

static uint64_t elf_do_rel(Elf64_Ehdr *hdr, Elf64_Rel *rel, Elf64_Shdr *reltab) {
    Elf64_Shdr *target = elf_section(hdr, reltab->sh_info);
    uint64_t addr = (uint64_t)hdr + target->sh_offset;
    uint64_t *ref = (uint64_t*)(addr + rel->r_offset);
    uint64_t addend = *ref;

    uint64_t symval = 0;
    if (ELF64_R_SYM(rel->r_info) != SHN_UNDEF) {
        symval = elf_get_symval(hdr, reltab->sh_link, ELF64_R_SYM(rel->r_info));
        if (symval == ELF_RELOC_ERR) return ELF_RELOC_ERR;
    }

    switch (ELF64_R_TYPE(rel->r_info)) {
        case R_AARCH64_ABS64:
            *ref = symval + addend;
            break;
        case R_AARCH64_RELATIVE: 
            *ref = (uint64_t)hdr + addend;
            break;
        case R_AARCH64_JUMP_SLOT:
            *ref = symval; 
            break;
        default:
            ERROR("Unsupported SHT_REL relocation type: %d\n", ELF64_R_TYPE(rel->r_info));
            return ELF_RELOC_ERR;
    }
    return symval;
}

# define ELF_PHDR_ERR -2



static int elf_load_stage2(Elf64_Ehdr *hdr) {
    Elf64_Shdr *shdr = elf_sheader(hdr);
    for (unsigned int i = 0; i < hdr->e_shnum; i++) {
        Elf64_Shdr *section = &shdr[i];
		switch (section->sh_type) {
			case SHT_RELA: {
				printf("Processing SHT_RELA section %d\n", i);
				const char *secname = elf_section_name(hdr, section);
				if (K::strcmp(secname, ".rela.dyn") == 0 || 
					K::strcmp(secname, ".rela.plt") == 0) {
					printf("Processing dynamic relocations in %s\n", secname);
					for (unsigned int idx = 0; idx < section->sh_size / section->sh_entsize; idx++) {
						Elf64_Rela *rel = (Elf64_Rela*)((uint64_t)hdr + section->sh_offset + idx * sizeof(Elf64_Rela));
						if (elf_do_dynamic_reloc(hdr, rel, section) == ELF_RELOC_ERR) {
							ERROR("Failed to apply dynamic relocation\n");
							return ELF_RELOC_ERR;
						}
					}
				}
			}
			case SHT_REL: {
				// printf("Processing SHT_REL section %d\n", i);
				// for (unsigned int idx = 0; idx < section->sh_size / section->sh_entsize; idx++) {
				// 	Elf64_Rel *rel = (Elf64_Rel*)((uint64_t)hdr + section->sh_offset + idx * sizeof(Elf64_Rel));
				// 	int result = elf_do_rel(hdr, rel, section);
				// 	if (result == ELF_RELOC_ERR) {
				// 		ERROR("Failed to relocate symbol (SHT_REL)\n");
				// 		return ELF_RELOC_ERR;
				// 	}
				// }
				// continue;
			}
		}
		// TODO: figure out what the hell other shtypes do
    }
    return 0;
}

void *load_segment_mem(void* mem, Elf64_Phdr *phdr, UserTCB* tcb) {
    size_t mem_size = phdr->p_memsz;
    off_t mem_offset = phdr->p_offset;
    size_t file_size = phdr->p_filesz;
    void *vaddr = (void *)(phdr->p_vaddr);
    
    // Ensure page alignment for mmap
    off_t page_offset = (uint64_t)vaddr % PAGE_SIZE;
    void* aligned_vaddr = (void*)((uint64_t)vaddr - page_offset);
    size_t aligned_size = mem_size + page_offset;

    // mmap the memory region with the correct protections
    int prot = 0;
    if (phdr->p_flags & PF_R) prot |= PROT_READ;
    if (phdr->p_flags & PF_W) prot |= PROT_WRITE;
    if (phdr->p_flags & PF_X) prot |= PROT_EXEC;
	printf("vaddr: %x, size: %x\n", (uint64_t)aligned_vaddr, aligned_size);
	void* to = (void*)((uint64_t)aligned_vaddr + page_offset);
	void* from = (void*)(mem_offset);
	printf("mapping 0x%x, from memory 0x%x\n", to, from);
	void* end = (void*)((uint64_t)from + mem_size);
	mmap(tcb, (uint64_t)aligned_vaddr, prot, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, nullptr, 0, aligned_size, 
		[=]() {
			load_mmapped_page(tcb, (uint64_t)aligned_vaddr, [=](uint64_t kvaddr) {
				tcb->page_table->use_page_table();
				void* page_to = (void*)kvaddr;
				if (page_to < to) {
					page_to = to;
				}
				void* page_from = ((uint64_t)page_to - (uint64_t)to) + mem;
				size_t cpy_size = PAGE_SIZE;
				if ((uint64_t)end - (uint64_t)page_from < cpy_size) {
					cpy_size = (size_t)((uint64_t)end - (uint64_t)page_from);
				}
				K::memcpy(page_to, page_from, cpy_size);
			});
		}
	);
    return (void*)vaddr;
}


void* load_library(char *name, UserTCB* tcb);


static int elf_load_stage3(Elf64_Ehdr *hdr, UserTCB* tcb) {
	Elf64_Phdr *phdr = elf_pheader(hdr);
	unsigned int i, idx;
	for (i = 0; i < hdr->e_phnum; i++) {
		Elf64_Phdr *prog = &phdr[i];
		switch(prog->p_type) {
			case PT_NULL:
				break;
			case PT_LOAD:
				// load this in
				load_segment_mem((void*) hdr, prog, tcb);
				break;
			case PT_DYNAMIC: {
				Elf64_Dyn *dyn = (Elf64_Dyn*)((uint64_t)hdr + prog->p_offset);
				uint64_t dynstr_addr = 0;	
				
				printf("%x, is dynstr_addr\n", dynstr_addr);
				for (; dyn->d_tag != DT_NULL; dyn++) {
					if (dyn->d_tag == DT_STRTAB) {
						dynstr_addr = dyn->d_un.d_ptr;
						break;
					}
				}
				printf("%x, is dynstr_addr\n", dynstr_addr);
				dyn = (Elf64_Dyn*)((uint64_t)hdr + prog->p_offset);
				for (; dyn->d_tag != DT_NULL; dyn++) {
					if (dyn->d_tag == DT_NEEDED) {
						char *libname = (char*)((uint64_t)hdr + dynstr_addr + dyn->d_un.d_val);
						printf("Loading dependency: %s\n", libname);
						void *lib = load_library(libname, tcb);
						if (!lib) {
							ERROR("Failed to load %s\n", libname);
							return ELF_RELOC_ERR;
						}
					}
				}
				break;
			}
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
				// load_segment_mem((void*) hdr, prog, tcb);
				break;
			default:
				ERROR("unsupported program header type.\n");
		}
	}
	return 0;
}

// load
static inline void *elf_load_rel(Elf64_Ehdr *hdr, UserTCB* tcb) {
	int result;
	result = elf_load_stage1(hdr, tcb);
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
	result = elf_load_stage3(hdr, tcb);
	if (result == ELF_PHDR_ERR) {
		ERROR("Unable to load ELF file.\n");
		return nullptr;
	}
	return (void *)hdr->e_entry;
}

void* load_library(char *name, UserTCB* tcb) {
    // check if already loaded
    for (LoadedLibrary *lib = g_loaded_libs; lib; lib = lib->next) {
        if (K::strcmp(lib->name, name) == 0) {
            return lib->ehdr;
        }
    }

    // load
    int file_index = get_ramfs_index(name);
    if (file_index < 0) {
        ERROR("Library %s not found\n", name);
        return nullptr;
    }

    // read into memory
    size_t size = ramfs_size(file_index);
    char *buffer = (char*) kmalloc(size);
    ramfs_read(buffer, 0, size, file_index);

    // load  elf for lib
    Elf64_Ehdr *lib_hdr = (Elf64_Ehdr*)buffer;
    if (!elf_check_supported(lib_hdr)) {
        ERROR("Invalid library: %s\n", name);
        kfree(buffer);
        return nullptr;
    }

    // add to global list
    LoadedLibrary *new_lib = (LoadedLibrary *) kmalloc(sizeof(LoadedLibrary));
    new_lib->ehdr = lib_hdr;
    new_lib->name = name;
    new_lib->next = g_loaded_libs;
    g_loaded_libs = new_lib;

    // relocate by stage
    elf_load_stage1(lib_hdr, tcb);
    elf_load_stage2(lib_hdr);
    elf_load_stage3(lib_hdr, tcb);

    return lib_hdr;
}

// load
static inline void *elf_load_exec(Elf64_Ehdr *hdr, UserTCB* tcb) {
	int result;
	result = elf_load_stage1(hdr, tcb);
	if(result == ELF_RELOC_ERR) {
		ERROR("Unable to load ELF file.\n");
		return nullptr;
	}
	// Parse the program header (if present)
	result = elf_load_stage3(hdr, tcb);
	if (result == ELF_PHDR_ERR) {
		ERROR("Unable to load ELF file.\n");
		return nullptr;
	}
	return (void *)hdr->e_entry;
}

void *elf_load(void* ptr, UserTCB* tcb) {
	Elf64_Ehdr *hdr = (Elf64_Ehdr *)ptr;
	if(!elf_check_supported(hdr)) {
		ERROR("ELF File cannot be loaded.\n");
		return nullptr;
	}
	switch(hdr->e_type) {
		case ET_EXEC:
			return elf_load_exec(hdr, tcb);
		case ET_REL:
			return elf_load_rel(hdr, tcb);
		case ET_DYN:
			return elf_load_rel(hdr, tcb);
			// todo ... ?? ? ? ? ? ??
	}
	return nullptr;
} 
