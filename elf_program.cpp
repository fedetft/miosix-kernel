
#include "elf_program.h"
#include <stdexcept>
#include <cstring>
#include <cstdio>

using namespace std;

//FIXME: move in miosix_config.h
static const int MAX_PROCESS_IMAGE_SIZE=20*1024;
static const int MIN_PROCESS_STACK_SIZE=2*1024;

///By convention, in an elf file for Miosix, the data segment starts @ this addr
static const unsigned int DATA_START=0x10000000;

//
// class ElfProgram
//

ElfProgram::ElfProgram(const unsigned int *elf, unsigned int size) : elf(elf)
{
    //Trying to follow the "full recognition before processing" approach,
    //(http://www.cs.dartmouth.edu/~sergey/langsec/occupy/FullRecognition.jpg)
    //all of the elf fields that will later be used are checked in advance
    if(validateHeader(size)==false) throw runtime_error("Bad file");
}

bool ElfProgram::validateHeader(unsigned int size)
{
    //Validate ELF header
    //Note: this code assumes a little endian elf and a little endian ARM CPU
    if(size<sizeof(Elf32_Ehdr)) return false;
    static const char magic[EI_NIDENT]={0x7f,'E','L','F',1,1,1};
    if(memcmp(elf,magic,EI_NIDENT)) throw runtime_error("Unrecognized format");
    const Elf32_Ehdr *ehdr=getElfHeader();
    if(ehdr->e_type!=ET_EXEC) throw runtime_error("Not an executable");
    if(ehdr->e_machine!=EM_ARM) throw runtime_error("Wrong CPU arch");
    if(ehdr->e_version!=EV_CURRENT) return false;
    if(ehdr->e_entry>=size) return false;
    if(ehdr->e_phoff>=size-sizeof(Elf32_Phdr)) return false;
    if(ehdr->e_flags!=(EF_ARM_EABI_MASK | EF_HAS_ENTRY_POINT)) return false;
    if(ehdr->e_ehsize!=sizeof(Elf32_Ehdr)) return false;
    if(ehdr->e_phentsize!=sizeof(Elf32_Phdr)) return false;
    //This to avoid that the next condition could pass due to 32bit wraparound
    //20 is an arbitrary number, could be increased if required
    if(ehdr->e_phnum>20) throw runtime_error("Too many segments");
    if(ehdr->e_phoff+(ehdr->e_phnum*sizeof(Elf32_Phdr))>size) return false;
    
    //Validate program header table
    bool codeSegmentPresent=false;
    bool dataSegmentPresent=false;
    bool dynamicSegmentPresent=false;
    const Elf32_Phdr *phdr=getProgramHeaderTable();
    int dataSegmentSize;
    for(int i=0;i<getNumOfProgramHeaderEntries();i++,phdr++)
    {
        //The third condition does not imply the other due to 32bit wraparound
        if(phdr->p_offset>=size) return false;
        if(phdr->p_filesz>=size) return false;
        if(phdr->p_offset+phdr->p_filesz>size) return false;
        
        if(phdr->p_align>8) throw runtime_error("Segment alignment too strict");
        
        switch(phdr->p_type)
        {
            case PT_DYNAMIC:
                if(dynamicSegmentPresent) return false; //Two dynamic segments?
                dynamicSegmentPresent=true;
                //DYNAMIC segment *must* come after data segment
                if(dataSegmentPresent==false) return false;
                if(validateDynamicSegment(phdr,size,dataSegmentSize)==false)
                    return false;
                break;
            case PT_LOAD:
                if(phdr->p_flags & ~(PF_R | PF_W | PF_X)) return false;
                if(!(phdr->p_flags & PF_R)) return false;
                if((phdr->p_flags & PF_W) && (phdr->p_flags & PF_X))
                    throw runtime_error("File violates W^X");
                if(phdr->p_flags & PF_X)
                {
                    if(codeSegmentPresent) return false; //Two code segments?
                    codeSegmentPresent=true;
                    if(ehdr->e_entry<phdr->p_offset ||
                       ehdr->e_entry>phdr->p_offset+phdr->p_filesz ||
                       phdr->p_filesz!=phdr->p_memsz) return false;
                }
                if(phdr->p_flags & PF_W)
                {
                    if(dataSegmentPresent) return false; //Two data segments?
                    dataSegmentPresent=true;
                    int maxSize=MAX_PROCESS_IMAGE_SIZE-MIN_PROCESS_STACK_SIZE;
                    if(phdr->p_memsz>=maxSize)
                        throw runtime_error("Data segment too big");
                    dataSegmentSize=phdr->p_memsz;
                }
                break;
            default:
                //Ignoring other segments
                break;
        }
    }
    if(codeSegmentPresent==false) return false; //Can't not have code segment
    return true;
}

bool ElfProgram::validateDynamicSegment(const Elf32_Phdr *dynamic,
        unsigned int size, unsigned int dataSegmentSize)
{
    unsigned int base=reinterpret_cast<unsigned int>(elf);
    const Elf32_Dyn *dyn=
        reinterpret_cast<const Elf32_Dyn*>(base+dynamic->p_offset);
    const int dynSize=dynamic->p_memsz/sizeof(Elf32_Dyn);
    Elf32_Addr dtRel;
    Elf32_Word dtRelsz;
    unsigned int hasRelocs=0;
    for(int i=0;i<dynSize;i++,dyn++)
    {
        switch(dyn->d_tag)
        {
            case DT_REL:
                hasRelocs |= 0x1;
                dtRel=dyn->d_un.d_ptr;
                break;
            case DT_RELSZ:
                hasRelocs |= 0x2;
                dtRelsz=dyn->d_un.d_val;
                break;
            case DT_RELENT:
                hasRelocs |= 0x4;
                if(dyn->d_un.d_val!=sizeof(Elf32_Rel)) return false;
                break;    
            case DT_RELA:
            case DT_RELASZ:
            case DT_RELAENT:
                throw runtime_error("RELA relocations unsupported");
            default:
                //Ignore other entries
                break;
        }
    }
    if(hasRelocs!=0 && hasRelocs!=0x7) return false;
    if(hasRelocs)
    {
        //The third condition does not imply the other due to 32bit wraparound
        if(dtRel>=size) return false;
        if(dtRelsz>=size) return false;
        if(dtRel+dtRelsz>size) return false;
        
        const Elf32_Rel *rel=reinterpret_cast<const Elf32_Rel*>(base+dtRel);
        const int relSize=dtRelsz/sizeof(Elf32_Rel);
        for(int i=0;i<relSize;i++,rel++)
        {
            switch(ELF32_R_TYPE(rel->r_info))
            {
                case R_ARM_NONE:
                    break;
                case R_ARM_ABS32:
                case R_ARM_RELATIVE:
                    if(rel->r_offset<DATA_START) return false;
                    if(rel->r_offset>DATA_START+dataSegmentSize-4) return false;
                    break;
                default:
                    throw runtime_error("Unexpected relocation type");
            }
        }
    }
    return true;
}

//
// class ProcessImage
//

void ProcessImage::load(ElfProgram& program)
{
    if(image) delete[] image;
    
    
}

ProcessImage::~ProcessImage()
{
    if(image) delete[] image;
}

