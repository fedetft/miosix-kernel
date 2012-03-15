/* 
 * File:   ELF.h
 * Author: lr
 *
 * Created on March 3, 2012, 3:31 PM
 */

#include <inttypes.h>

#ifndef ELF_TYPES_H
#define	ELF_TYPES_H

typedef uint32_t Elf32_Word;
typedef int32_t  Elf32_Sword;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef uint32_t Elf32_Addr;

static const int EI_NIDENT=16;

struct Elf32_Ehdr
{
    unsigned char e_ident[EI_NIDENT]; // Ident bytes
    Elf32_Half e_type;                // File type, any of the ET_* constants
    Elf32_Half e_machine;             // Target machine
    Elf32_Word e_version;             // File version
    Elf32_Addr e_entry;               // Start address
    Elf32_Off e_phoff;                // Phdr file offset
    Elf32_Off e_shoff;                // Shdr file offset
    Elf32_Word e_flags;               // File flags
    Elf32_Half e_ehsize;              // Sizeof ehdr
    Elf32_Half e_phentsize;           // Sizeof phdr
    Elf32_Half e_phnum;               // Number phdrs
    Elf32_Half e_shentsize;           // Sizeof shdr
    Elf32_Half e_shnum;               // Number shdrs
    Elf32_Half e_shstrndx;            // Shdr string index
} __attribute__((packed));

// Values for e_type
static const int ET_NONE = 0; // Unknown type
static const int ET_REL  = 1; // Relocatable
static const int ET_EXEC = 2; // Executable
static const int ET_DYN  = 3; // Shared object
static const int ET_CORE = 4; // Core file

// Values for e_version
static const int EV_CURRENT = 1;

// Values for e_machine
static const int EM_ARM  = 0x28;

// Values for e_flags
static const int EF_ARM_EABI_MASK = 0x05000000;
static const int EF_HAS_ENTRY_POINT = 2;

struct Elf32_Phdr
{
    Elf32_Word p_type;   // Program header type, any of the PH_* constants
    Elf32_Off  p_offset; // Segment start offset in file
    Elf32_Addr p_vaddr;  // Segment virtual address
    Elf32_Addr p_paddr;  // Segment physical address
    Elf32_Word p_filesz; // Segment size in file
    Elf32_Word p_memsz;  // Segment size in memory
    Elf32_Word p_flags;  // Segment flasgs, any of the PF_* constants
    Elf32_Word p_align;  // Segment alignment requirements
} __attribute__((packed));

// Values for p_type
static const int PT_NULL    = 0; // Unused array entry
static const int PT_LOAD    = 1; // Loadable segment
static const int PT_DYNAMIC = 2; // Segment is the dynamic section with relocs
static const int PT_INTERP  = 3; // Shared library interpreter
static const int PT_NOTE    = 4; // Auxiliary information

// Values for p_flags
static const int PF_X = 0x1; // Execute
static const int PF_W = 0x2; // Write
static const int PF_R = 0x4; // Read

struct Elf32_Dyn
{
    Elf32_Sword d_tag;
    union {
        Elf32_Word d_val;
        Elf32_Addr d_ptr;
    } d_un;
} __attribute__((packed));

// Values for d_tag
static const int DT_NULL     = 0;
static const int DT_NEEDED   = 1;
static const int DT_PLTRELSZ = 2;
static const int DT_PLTGOT   = 3;
static const int DT_HASH     = 4;
static const int DT_STRTAB   = 5;
static const int DT_SYMTAB   = 6;
static const int DT_RELA     = 7;
static const int DT_RELASZ   = 8;
static const int DT_RELAENT  = 9;
static const int DT_STRSZ    = 10;
static const int DT_SYMENT   = 11;
static const int DT_INIT     = 12;
static const int DT_FINI     = 13;
static const int DT_SONAME   = 14;
static const int DT_RPATH    = 15;
static const int DT_SYMBOLIC = 16;
static const int DT_REL      = 17;
static const int DT_RELSZ    = 18;
static const int DT_RELENT   = 19;
static const int DT_PLTREL   = 20;
static const int DT_DEBUG    = 21;
static const int DT_TEXTREL  = 22;
static const int DT_JMPREL   = 23;
static const int DT_BINDNOW  = 24;

struct Elf32_Rel
{
    Elf32_Addr r_offset;
    Elf32_Word r_info;
} __attribute__((packed));

// To extract the two fields of r_info
#define ELF32_R_SYM(i) ((i)>>8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))


// Possible values for ELF32_R_TYPE(r_info)
static const int R_ARM_NONE     = 0;
static const int R_ARM_ABS32    = 2;
static const int R_ARM_RELATIVE = 23;

#endif //ELF_TYPES_H
