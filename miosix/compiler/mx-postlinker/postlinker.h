/***************************************************************************
 *   Copyright (C) 2012 by Luigi Rucco and Terraneo Federico               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef POSTLINKER_H
#define	POSTLINKER_H

#include "ELF.h"
#include <cstring>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <utility>
#include <cstdlib>
#include <fstream>

/**
 * This class performs transformations on an elf file,
 * including stripping the section header and associated
 * string table, and setting some Miosix specific options
 * in the dynamic segment
 */
class PostLinker
{
public:
    /**
     * Constructor
     * \param s elf file name 
     */
    PostLinker(std::string s);

    /**
     * Remove the section header and string table from the elf file
     */
    void removeSectionHeaders();

    /**
     * Set the Miosix specific options in the dynamic segment
     * \param stackSize size that the runtime linker-loader will reserve for
     * the stack of the process
     * \param ramSize size of the process RAM image that the runtime
     * linker-loader will allocate for the process
     */
    void setMxTags(int stackSize, int ramSize);

    /**
     * Write changes to disk
     */
    void writeFile();

    /**
     * Destructor
     */
    ~PostLinker();
    
private:
    PostLinker(const PostLinker&);
    PostLinker& operator= (const PostLinker&);

    /**
     * \return the elf header
     */
    Elf32_Ehdr* getElfHeader()
    {
        return reinterpret_cast<Elf32_Ehdr*>(elf);
    }

    /**
     * Allows to retrieve a section header given its index
     * \param index a index from 0 to getElfHeader()->e_shnum
     * \return the corresponding section header
     */
    Elf32_Shdr* getElfSection(int index)
    {
        unsigned char *base=reinterpret_cast<unsigned char*>(elf);
        unsigned int offset=getElfHeader()->e_shoff+index*sizeof(Elf32_Shdr);
        if(offset+sizeof(Elf32_Shdr)>size)
            throw std::runtime_error("Elf section outside file bounds");
        return reinterpret_cast<Elf32_Shdr*>(base+offset);
    }
    
    /**
     * Allows to retrieve a segment header given its index
     * \param index a index from 0 to getElfHeader()->e_phnum
     * \return the corresponding secgment header
     */
    Elf32_Phdr *getElfSegment(int index)
    {
        unsigned char *base=reinterpret_cast<unsigned char*>(elf);
        unsigned int offset=getElfHeader()->e_phoff+index*sizeof(Elf32_Phdr);
        if(offset+sizeof(Elf32_Phdr)>size)
            throw std::runtime_error("Elf segment outside file bounds");
        return reinterpret_cast<Elf32_Phdr*>(base+offset);
    }
    
    /**
     * \return the size of the segment that is loaded in RAM, with
     * .data and .bss
     */
    int getSizeOfDataAndBss();
    
    /**
     * \return a pair with a pointer to the first element in the dynamic
     * segment and the number of entries in the dynamic segment
     */
    std::pair<Elf32_Dyn *,Elf32_Word> getDynamic();
    
    Elf32_Word* elf;
    int size;
    int newSize;
    std::string elfFile;
};

#endif	//POSTLINKER_H
