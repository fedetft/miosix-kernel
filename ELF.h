/* 
 * File:   ELF.h
 * Author: lr
 *
 * Created on March 3, 2012, 3:31 PM
 */

#ifndef ELF_H
#define	ELF_H

#define EI_NIDENT 16

typedef unsigned long  elf32_word;
typedef   signed long  elf32_sword;
typedef unsigned short elf32_half;
typedef unsigned long  elf32_off;
typedef unsigned long  elf32_addr;

struct elf32_ehdr {
  unsigned char e_ident[EI_NIDENT];    /* ident bytes */
  elf32_half e_type;                   /* file type */
  elf32_half e_machine;                /* target machine */
  elf32_word e_version;                /* file version */
  elf32_addr e_entry;                  /* start address */
  elf32_off e_phoff;                   /* phdr file offset */
  elf32_off e_shoff;                   /* shdr file offset */
  elf32_word e_flags;                  /* file flags */
  elf32_half e_ehsize;                 /* sizeof ehdr */
  elf32_half e_phentsize;              /* sizeof phdr */
  elf32_half e_phnum;                  /* number phdrs */
  elf32_half e_shentsize;              /* sizeof shdr */
  elf32_half e_shnum;                  /* number shdrs */
  elf32_half e_shstrndx;               /* shdr string index */
};

// Values for e_type
#define ET_NONE         0       /* Unknown type. */
#define ET_REL          1       /* Relocatable. */
#define ET_EXEC         2       /* Executable. */
#define ET_DYN          3       /* Shared object. */
#define ET_CORE         4       /* Core file. */

//values for e_machine

struct elf32_shdr {
  elf32_word sh_name; 		
  elf32_word sh_type; 		
  elf32_word sh_flags; 	        
  elf32_addr sh_addr; 		
  elf32_off  sh_offset; 	        
  elf32_word sh_size; 		
  elf32_word sh_link; 		
  elf32_word sh_info; 		
  elf32_word sh_addralign; 	
  elf32_word sh_entsize; 	
};

// sh_type 
#define SHT_NULL        0               /* inactive */
#define SHT_PROGBITS    1               /* program defined information */
#define SHT_SYMTAB      2               /* symbol table section */   
#define SHT_STRTAB      3               /* string table section */
#define SHT_RELA        4               /* relocation section with addends*/
#define SHT_HASH        5               /* symbol hash table section */
#define SHT_DYNAMIC     6               /* dynamic section */
#define SHT_NOTE        7               /* note section */
#define SHT_NOBITS      8               /* no space section */
#define SHT_REL         9               /* relation section without addends */
#define SHT_SHLIB       10              /* reserved - purpose unknown */
#define SHT_DYNSYM      11              /* dynamic symbol table section */
#define SHT_LOPROC      0x70000000      /* reserved range for processor */
#define SHT_HIPROC      0x7fffffff      /* specific section header types */
#define SHT_LOUSER      0x80000000      /* reserved range for application */
#define SHT_HIUSER      0xffffffff      /* specific indexes */

struct elf32_rel {
  elf32_addr      r_offset;       
  elf32_word      r_info;         
};

struct elf32_rela {
  elf32_addr      r_offset;       
  elf32_word      r_info;         
  elf32_sword     r_addend;       
};

struct elf32_sym {
  elf32_word      st_name;        
  elf32_addr      st_value;       
  elf32_word      st_size;        
  unsigned char   st_info;        
  unsigned char   st_other;       
  elf32_half      st_shndx;       
};


#endif	/* ELF_H */

