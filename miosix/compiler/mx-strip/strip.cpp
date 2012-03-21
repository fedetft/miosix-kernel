/* 
 * File:   Strip.cpp
 * Author: lr
 * 
 * Created on March 21, 2012, 2:26 PM
 */

#include "strip.h"
#include <stdexcept>

using namespace std;

Strip::Strip(string s) 
{
    elfFile=s;
    ifstream f(s.c_str(),ios::binary);
    f.seekg (0, ios::end);
    size=f.tellg();
    newSize=size;
    f.seekg (0, ios::beg);
    int roundedSize=(size+sizeof(int)-1) & ~(sizeof(int)-1);
    elf=new int[roundedSize/sizeof(int)];
    memset(elf,0,roundedSize);
    f.read(reinterpret_cast<char*>(elf),size);
    getElfHeader()->e_type=ET_EXEC;
}


Strip::~Strip() 
{
    delete[] elf;
}

void Strip::removeSectionHeaders()
{ 
    newSize=getElfSection(getElfHeader()->e_shstrndx)->sh_offset;
    getElfHeader()->e_shoff=0;
    getElfHeader()->e_shnum=0;
    getElfHeader()->e_shentsize=0;
    getElfHeader()->e_shstrndx=0;
}

void Strip::setMxTags(int stackSize, int ramSize)
{
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

void Strip::writeFile()
{
    ofstream o(elfFile.c_str(),ios::binary);
    o.write(reinterpret_cast<char*>(elf),newSize);
}

pair<Elf32_Dyn *,Elf32_Word> Strip::getDynamic()
{
    for(int i=0;i<getElfHeader()->e_phnum;i++)
    {
        Elf32_Phdr* phdr=getProgramHeaderTable(i);
        if(phdr->p_type!=PT_DYNAMIC) continue;
        unsigned int base=reinterpret_cast<unsigned int>(elf);
        return make_pair(reinterpret_cast<Elf32_Dyn*>(base+phdr->p_offset),
                phdr->p_memsz/sizeof(Elf32_Dyn));
    }
    throw runtime_error("Dynamic not found");
}
