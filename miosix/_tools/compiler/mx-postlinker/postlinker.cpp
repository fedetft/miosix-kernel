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

#include "postlinker.h"

using namespace std;

PostLinker::PostLinker(string s) 
{
    elfFile=s;
    ifstream f(s.c_str(),ios::binary);
    if(!f.good()) throw runtime_error("File not found");
    f.seekg(0,ios::end);
    size=f.tellg();
    newSize=size;
    f.seekg(0,ios::beg);
    int roundedSize=(size+sizeof(Elf32_Word)-1) & ~(sizeof(Elf32_Word)-1);
    elf=new Elf32_Word[roundedSize/sizeof(Elf32_Word)];
    memset(elf,0,roundedSize);
    f.read(reinterpret_cast<char*>(elf),size);
    static const char magic[EI_NIDENT]={0x7f,'E','L','F',1,1,1};
    if(size<sizeof(Elf32_Ehdr) || memcmp(getElfHeader()->e_ident,magic,EI_NIDENT))
        throw runtime_error("Unrecognized format");
}

void PostLinker::removeSectionHeaders()
{ 
    newSize=getElfSection(getElfHeader()->e_shstrndx)->sh_offset;
    getElfHeader()->e_shoff=0;
    getElfHeader()->e_shnum=0;
    getElfHeader()->e_shentsize=0;
    getElfHeader()->e_shstrndx=0;
}

void PostLinker::setMxTags(int stackSize, int ramSize)
{
    if(stackSize & 0x3)
        throw runtime_error("stack size not four word aligned");
    if(ramSize & 0x3)
        throw runtime_error("ram size not four word aligned");
    if(getSizeOfDataAndBss()+stackSize>ramSize)
        throw runtime_error(".data + .bss + stack exceeds ramsize");
    getElfHeader()->e_type=ET_EXEC; //Force ET_EXEC
    int ctr=0;
    pair<Elf32_Dyn *,Elf32_Word> dyn=getDynamic();
    for(int i=0;i<dyn.second;i++,dyn.first++)
    {
        if(dyn.first->d_tag!=DT_NULL) continue;
        switch(ctr)
        {
            case 0:
                dyn.first->d_tag=DT_MX_RAMSIZE;
                dyn.first->d_un.d_val=ramSize;
                break;
            case 1:
                dyn.first->d_tag=DT_MX_STACKSIZE;
                dyn.first->d_un.d_val=stackSize;
                break;
            case 2:
                dyn.first->d_tag=DT_MX_ABI;
                dyn.first->d_un.d_val=DV_MX_ABI_V0;
                return;
        }
        ctr++;     
    }
    throw runtime_error("Not enough null entries");
}

void PostLinker::writeFile()
{
    ofstream o(elfFile.c_str(),ios::binary);
    o.write(reinterpret_cast<char*>(elf),newSize);
}

pair<Elf32_Dyn *,Elf32_Word> PostLinker::getDynamic()
{
    for(int i=0;i<getElfHeader()->e_phnum;i++)
    {
        Elf32_Phdr* phdr=getElfSegment(i);
        if(phdr->p_type!=PT_DYNAMIC) continue;
        unsigned char *base=reinterpret_cast<unsigned char*>(elf);
        unsigned int offset=phdr->p_offset;
        if(offset+phdr->p_memsz>size)
            throw std::runtime_error("Dynamic outside file bounds");
        return make_pair(reinterpret_cast<Elf32_Dyn*>(base+offset),
                phdr->p_memsz/sizeof(Elf32_Dyn));
    }
    throw runtime_error("Dynamic not found");
}

int PostLinker::getSizeOfDataAndBss()
{
    for(int i=0;i<getElfHeader()->e_phnum;i++)
    {
        Elf32_Phdr* phdr=getElfSegment(i);
        if(phdr->p_type!=PT_LOAD) continue;
        if(!(phdr->p_flags & PF_W) || (phdr->p_flags & PF_X)) continue;
        return phdr->p_memsz;
    }
    throw runtime_error(".data/.bss not found");
}

PostLinker::~PostLinker() 
{
    delete[] elf;
}
