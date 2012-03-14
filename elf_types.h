/* 
 * File:   ELF.h
 * Author: lr
 *
 * Created on March 3, 2012, 3:31 PM
 */

#ifndef ELF_TYPES_H
#define	ELF_TYPES_H

typedef unsigned long  Elf32_Word;
typedef   signed long  Elf32_Sword;
typedef unsigned short Elf32_Half;
typedef unsigned long  Elf32_Off;
typedef unsigned long  Elf32_Addr;

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

//FIXME: cleanup
#define R_ARM_NONE              0       /* No reloc */
#define R_ARM_PC24              1       /* PC relative 26 bit branch */
#define R_ARM_ABS32             2       /* Direct 32 bit  */
#define R_ARM_REL32             3       /* PC relative 32 bit */
#define R_ARM_PC13              4
#define R_ARM_ABS16             5       /* Direct 16 bit */
#define R_ARM_ABS12             6       /* Direct 12 bit */
#define R_ARM_THM_ABS5          7
#define R_ARM_ABS8              8       /* Direct 8 bit */
#define R_ARM_SBREL32           9
#define R_ARM_THM_PC22          10
#define R_ARM_THM_PC8           11
#define R_ARM_AMP_VCALL9        12
#define R_ARM_SWI24             13
#define R_ARM_THM_SWI8          14
#define R_ARM_XPC25             15
#define R_ARM_THM_XPC22         16
#define R_ARM_TLS_DTPMOD32      17      /* ID of module containing symbol */
#define R_ARM_TLS_DTPOFF32      18      /* Offset in TLS block */
#define R_ARM_TLS_TPOFF32       19      /* Offset in static TLS block */
#define R_ARM_COPY              20      /* Copy symbol at runtime */
#define R_ARM_GLOB_DAT          21      /* Create GOT entry */
#define R_ARM_JUMP_SLOT         22      /* Create PLT entry */
#define R_ARM_RELATIVE          23      /* Adjust by program base */
#define R_ARM_GOTOFF            24      /* 32 bit offset to GOT */
#define R_ARM_GOTPC             25      /* 32 bit PC relative offset to GOT */
#define R_ARM_GOT32             26      /* 32 bit GOT entry */
#define R_ARM_PLT32             27      /* 32 bit PLT address */
#define R_ARM_ALU_PCREL_7_0     32
#define R_ARM_ALU_PCREL_15_8    33
#define R_ARM_ALU_PCREL_23_15   34
#define R_ARM_LDR_SBREL_11_0    35
#define R_ARM_ALU_SBREL_19_12   36
#define R_ARM_ALU_SBREL_27_20   37
#define R_ARM_GNU_VTENTRY       100
#define R_ARM_GNU_VTINHERIT     101
#define R_ARM_THM_PC11          102     /* thumb unconditional branch */
#define R_ARM_THM_PC9           103     /* thumb conditional branch */
#define R_ARM_TLS_GD32          104     /* PC-rel 32 bit for global dynamic
                                           thread local data */
#define R_ARM_TLS_LDM32         105     /* PC-rel 32 bit for local dynamic
                                           thread local data */
#define R_ARM_TLS_LDO32         106     /* 32 bit offset relative to TLS
                                           block */
#define R_ARM_TLS_IE32          107     /* PC-rel 32 bit for GOT entry of
                                           static TLS block offset */
#define R_ARM_TLS_LE32          108     /* 32 bit offset relative to static
                                           TLS block */
#define R_ARM_RXPC25            249
#define R_ARM_RSBREL32          250
#define R_ARM_THM_RPC22         251
#define R_ARM_RREL32            252
#define R_ARM_RABS22            253
#define R_ARM_RPC24             254
#define R_ARM_RBASE             255
/* Keep this the last entry.  */
#define R_ARM_NUM               256

#endif //ELF_TYPES_H
