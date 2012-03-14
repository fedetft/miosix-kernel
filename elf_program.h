
#include <utility>
#include "elf_types.h"

#ifndef ELF_PROGRAM_H
#define	ELF_PROGRAM_H

/**
 * This class represents an elf file.
 */
class ElfProgram
{
public:
    /**
     * Constructor
     * \param elf pointer to the elf file's content. Ownership of the data
     * remains of the caller, that is, the pointer is not deleted by this
     * class. This is done to allow passing a pointer directly to a location
     * in the microcontroller's FLASH memory, in order to avoid copying the
     * elf in RAM
     * \param size size of the content of the elf file
     */
    ElfProgram(const unsigned int *elf, int size);
    
    /**
     * \return the a pointer to the elf header
     */
    const Elf32_Ehdr *getElfHeader() const
    {
        return reinterpret_cast<const Elf32_Ehdr*>(elf);
    }
    
    /**
     * \return the already relocated value of the entry point 
     */
    unsigned int getEntryPoint() const
    {
        unsigned int base=reinterpret_cast<unsigned int>(elf);
        return base+getElfHeader()->e_entry;
    }
    
    /**
     * \return an array of struct Elf32_Phdr
     */
    const Elf32_Phdr *getProgramHeaderTable() const
    {
        unsigned int base=reinterpret_cast<unsigned int>(elf);
        return reinterpret_cast<const Elf32_Phdr*>(base+getElfHeader()->e_phoff);
    }
    
    /**
     * \return the number of entries in the program header table
     */
    int getNumOfProgramHeaderEntries() const
    {
        return getElfHeader()->e_phnum;
    }
    
private:
    const unsigned int * const elf; //Pointer to the content of the elf file
    const int size;                 //Size of the content of the elf file
};

/**
 * This class represent the RAM image of a process.
 */
class ProcessImage
{
public:
    /**
     * Constructor, creates an empty process image.
     */
    ProcessImage() : image(0), size(0) {}
    
    /**
     * Starting from the content of the elf program, create an image in RAM of
     * the process, including copying .data, zeroing .bss and performing
     * relocations
     */
    void load(ElfProgram& program);
    
    /**
     * Destructor. Deletes the process image memory.
     */
    ~ProcessImage();
    
private:
    ProcessImage(const ProcessImage&);
    ProcessImage& operator= (const ProcessImage&);
    
    unsigned int *image; //Pointer to the process image in RAM
    unsigned int size;   //Size of the process image
};

#endif //ELF_PROGRAM_H
