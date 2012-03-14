
#include <iostream>
#include <fstream>
#include "elf_program.h"

using namespace std;

int main()
{
    ifstream in("example.elf",ios::binary);
    in.seekg(0,ios::end);
    const int size=in.tellg();
    in.seekg(0,ios::beg);
    const int alignedSize=(size+3) & ~3;
    unsigned int *elfFileData=new unsigned int[alignedSize/4];
    in.read(reinterpret_cast<char*>(elfFileData),size);
    
    ElfProgram program(elfFileData,size);
    cout<<"Elf info:"<<endl
        <<"base address = "<<elfFileData<<endl
        <<"entry (file relative) = "<<program.getElfHeader()->e_entry<<endl
        <<"entry (relocated) = "<<program.getEntryPoint()<<endl;
    
    const Elf32_Phdr *hdr=program.getProgramHeaderTable();
    for(int i=0;i<program.getNumOfProgramHeaderEntries();i++,hdr++)
    {
        switch(hdr->p_type)
        {
            case PT_DYNAMIC:
                printf("Entry %d is dynamic\n",i);
                break;
            case PT_LOAD:
                printf("Entry %d is load\n",i);
                break;
            default:
                printf("Unexpected\n");
        }
        printf("Offset in file=%d\n",hdr->p_offset);
        printf("Virtual address=%d\n",hdr->p_vaddr);
        printf("Physical address=%d\n",hdr->p_paddr);
        printf("Size in file=%d\n",hdr->p_filesz);
        printf("Size in memory=%d\n",hdr->p_memsz);
        printf("Flags=%d\n",hdr->p_flags);
        printf("Align=%d\n",hdr->p_align);
    } 
}
