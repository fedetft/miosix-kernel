/* 
 * File:   Strip.h
 * Author: lr
 *
 * Created on March 21, 2012, 2:26 PM
 */

#ifndef STRIP_H
#define	STRIP_H

#include "ELF.h"
#include <string.h>
#include <iostream>
#include <exception>
#include <utility>
#include <stdlib.h>
#include <fstream>

class Strip
{
public:
    Strip(std::string s);
    ~Strip();
    void removeSectionHeaders();
    void setMxTags(int stackSize, int ramSize);
    void writeFile();
    
private:    
    Elf32_Ehdr* getElfHeader()
    {
        return reinterpret_cast<Elf32_Ehdr*>(elf);
    }

    Elf32_Shdr* getElfSection(int index)
    {
        unsigned int base=reinterpret_cast<unsigned int>(elf);
        return reinterpret_cast<Elf32_Shdr*>(base+getElfHeader()->
                e_shoff+index*sizeof(Elf32_Shdr));
    }
    
    /**
     * \return an array of struct Elf32_Phdr
     */
    Elf32_Phdr *getProgramHeaderTable(int index)
    {
        unsigned int base=reinterpret_cast<unsigned int>(elf);
        return reinterpret_cast<Elf32_Phdr*>(base+getElfHeader()->e_phoff+
                index*sizeof(Elf32_Phdr));
    }
    
    std::pair<Elf32_Dyn *,Elf32_Word> getDynamic();
    
    int* elf;
    int size;
    int newSize;
    std::string elfFile;
};

#endif	/* STRIP_H */
