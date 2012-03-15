
#include <iostream>
#include <fstream>
#include "elf_program.h"

using namespace std;

int main()
{
    ifstream in("example.elf",ios::binary);
    if(!in.good()) return 1;
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
}
