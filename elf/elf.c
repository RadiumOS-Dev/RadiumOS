#include "elf.h"
#include "../utility/utility.h"
#include "../terminal/terminal.h"
// Simple macros for logging (replace with your system's)
#define ERROR(fmt, ...) printr( "ELF ERROR: " fmt, ##__VA_ARGS__)
#define DEBUG(fmt, ...) printr("ELF DEBUG: " fmt, ##__VA_ARGS__)

// Stub for external symbol lookup (always returns NULL as per tutorial)
void* elf_lookup_symbol(const char* name) {
    (void)name;  // Unused
    return NULL;
}

// Check ELF magic number
bool elf_check_file(Elf32_Ehdr* hdr) {
    if (!hdr) return false;
    if (hdr->e_ident[EI_MAG0] != ELFMAG0) {
        ERROR("ELF Header EI_MAG0 incorrect.\n");
        return false;
    }
    if (hdr->e_ident[EI_MAG1] != ELFMAG1) {
        ERROR("ELF Header EI_MAG1 incorrect.\n");
        return false;
    }
    if (hdr->e_ident[EI_MAG2] != ELFMAG2) {
        ERROR("ELF Header EI_MAG2 incorrect.\n");
        return false;
    }
    if (hdr->e_ident[EI_MAG3] != ELFMAG3) {
        ERROR("ELF Header EI_MAG3 incorrect.\n");
        return false;
    }
    return true;
}

// Check if ELF is supported (i386, little-endian, 32-bit, rel/exec)
bool elf_check_supported(Elf32_Ehdr* hdr) {
    if (!elf_check_file(hdr)) {
        ERROR("Invalid ELF File.\n");
        return false;
    }
    if (hdr->e_ident[EI_CLASS] != ELFCLASS32) {
        ERROR("Unsupported ELF File Class.\n");
        return false;
    }
    if (hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        ERROR("Unsupported ELF File byte order.\n");
        return false;
    }
    if (hdr->e_machine != EM_386) {
        ERROR("Unsupported ELF File target.\n");
        return false;
    }
    if (hdr->e_ident[EI_VERSION] != EV_CURRENT) {
        ERROR("Unsupported ELF File version.\n");
        return false;
    }
    if (hdr->e_type != ET_REL && hdr->e_type != ET_EXEC) {
        ERROR("Unsupported ELF File type.\n");
        return false;
    }
    return true;
}

// Allocate SHT_NOBITS sections (e.g., BSS) - Stage 1
int elf_load_stage1(Elf32_Ehdr* hdr) {
    Elf32_Shdr* shdr = elf_sheader(hdr);
    unsigned int i;
    for (i = 0; i < hdr->e_shnum; i++) {
        Elf32_Shdr* section = &shdr[i];
        if (section->sh_type == SHT_NOBITS) {
            if (!section->sh_size) continue;
            if (section->sh_flags & SHF_ALLOC) {
                void* mem = malloc(section->sh_size);
                if (!mem) {
                    ERROR("Failed to allocate memory for section.\n");
                    return ELF_RELOC_ERR;
                }
                memset(mem, 0, section->sh_size);
                // Adjust sh_offset to point to allocated memory (relative to hdr)
                section->sh_offset = (Elf32_Off)((uintptr_t)mem - (uintptr_t)hdr);
                DEBUG("Allocated memory for a section (%u bytes).\n", section->sh_size);
            }
        }
    }
    return 0;
}

// Process relocations - Stage 2
int elf_load_stage2(Elf32_Ehdr* hdr) {
    Elf32_Shdr* shdr = elf_sheader(hdr);
    unsigned int i, idx;
    for (i = 0; i < hdr->e_shnum; i++) {
        Elf32_Shdr* section = &shdr[i];
        if (section->sh_type == SHT_REL) {  // Extend for SHT_RELA if needed
            for (idx = 0; idx < section->sh_size / section->sh_entsize; idx++) {
                Elf32_Rel* rel = &((Elf32_Rel*)((uintptr_t)hdr + section->sh_offset))[idx];
                int result = elf_do_reloc(hdr, rel, section);
                if (result == ELF_RELOC_ERR) {
                    ERROR("Failed to relocate symbol.\n");
                    return ELF_RELOC_ERR;
                }
            }
        }
    }
    return 0;
}

// Macros for relocation calculations
#define DO_386_32(S, A)  ((S) + (A))
#define DO_386_PC32(S, A, P) ((S) + (A) - (P))

// Perform a single relocation
int elf_do_reloc(Elf32_Ehdr* hdr, Elf32_Rel* rel, Elf32_Shdr* reltab) {
    Elf32_Shdr* target = elf_section(hdr, reltab->sh_info);
    uintptr_t addr = (uintptr_t)hdr + target->sh_offset;
    int32_t* ref = (int32_t*)(addr + rel->r_offset);

    // Get symbol value
    int symval = 0;
    uint32_t sym_idx = ELF32_R_SYM(rel->r_info);
    if (sym_idx != SHN_UNDEF) {
        symval = elf_get_symval(hdr, reltab->sh_link, sym_idx);
        if (symval == ELF_RELOC_ERR) return ELF_RELOC_ERR;
    }

    // Relocate based on type
    switch (ELF32_R_TYPE(rel->r_info)) {
        case R_386_NONE:
            // No relocation
            break;
        case R_386_32:
            *ref = DO_386_32(symval, *ref);
            break;
        case R_386_PC32:
            *ref = DO_386_PC32(symval, *ref, (int)(uintptr_t)ref);
            break;
        default:
            ERROR("Unsupported Relocation Type (%d).\n", ELF32_R_TYPE(rel->r_info));
            return ELF_RELOC_ERR;
    }
    return symval;
}

// Get symbol value (absolute address)
int elf_get_symval(Elf32_Ehdr* hdr, int table, uint32_t idx) {
    if (table == SHN_UNDEF || idx == SHN_UNDEF) return 0;
    Elf32_Shdr* symtab = elf_section(hdr, table);
    uint32_t symtab_entries = symtab->sh_size / symtab->sh_entsize;
    if (idx >= symtab_entries) {
        ERROR("Symbol Index out of Range (%d:%u).\n", table, idx);
        return ELF_RELOC_ERR;
    }

    uintptr_t symaddr = (uintptr_t)hdr + symtab->sh_offset;
    Elf32_Sym* symbol = &((Elf32_Sym*)symaddr)[idx];

    if (symbol->st_shndx == SHN_UNDEF) {
        // External symbol, lookup value
        Elf32_Shdr* strtab = elf_section(hdr, symtab->sh_link);
        const char* name = (const char*)((uintptr_t)hdr + strtab->sh_offset + symbol->st_name);
        void* target = elf_lookup_symbol(name);
        if (target == NULL) {
            // Extern symbol not found
            if (ELF32_ST_BIND(symbol->st_info) == STB_WEAK) {
                // Weak symbol initialized as 0
                return 0;
            } else {
                ERROR("Undefined External Symbol: %s.\n", name);
                return ELF_RELOC_ERR;
            }
        } else {
            return (int)(uintptr_t)target;
        }
    } else if (symbol->st_shndx == SHN_ABS) {
        // Absolute symbol
        return symbol->st_value;
    } else {
        // Internally defined symbol
        Elf32_Shdr* target_sec = elf_section(hdr, symbol->st_shndx);
        return (int)((uintptr_t)hdr + symbol->st_value + target_sec->sh_offset);
    }
    return 0;  // Fallback
}

// Convenience functions for sections and strings
Elf32_Shdr* elf_sheader(Elf32_Ehdr* hdr) {
    return (Elf32_Shdr*)((uintptr_t)hdr + hdr->e_shoff);
}

Elf32_Shdr* elf_section(Elf32_Ehdr* hdr, int idx) {
    return &elf_sheader(hdr)[idx];
}

char* elf_str_table(Elf32_Ehdr* hdr) {
    if (hdr->e_shstrndx == SHN_UNDEF) return NULL;
    return (char*)((uintptr_t)hdr + elf_section(hdr, hdr->e_shstrndx)->sh_offset);
}

char* elf_lookup_string(Elf32_Ehdr* hdr, int offset) {
    char* strtab = elf_str_table(hdr);
    if (strtab == NULL) return NULL;
    return strtab + offset;
}

// Load relocatable ELF (stages 1+2, return entry point)
void* elf_load_rel(Elf32_Ehdr* hdr) {
    int result = elf_load_stage1(hdr);
    if (result == ELF_RELOC_ERR) {
        ERROR("Unable to load ELF file (stage 1).\n");
        return NULL;
    }
    result = elf_load_stage2(hdr);
    if (result == ELF_RELOC_ERR) {
        ERROR("Unable to load ELF file (stage 2).\n");
        return NULL;
    }
    // TODO: Parse program header if present for entry adjustment
    return (void*)(uintptr_t)hdr->e_entry;
}

// Main entry: Load ELF file from memory
void* elf_load_file(void* file) {
    Elf32_Ehdr* hdr = (Elf32_Ehdr*)file;
    if (!elf_check_supported(hdr)) {
        ERROR("ELF File cannot be loaded.\n");
        return NULL;
    }
    switch (hdr->e_type) {
        case ET_EXEC:
            // TODO: Implement executable loading (e.g., via program headers)
            ERROR("ET_EXEC loading not implemented.\n");
            return NULL;
        case ET_REL:
            return elf_load_rel(hdr);
        default:
            return NULL;
    }
    return NULL;
}