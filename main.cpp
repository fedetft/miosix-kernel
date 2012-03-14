
#include <cstdio>
#include <cassert>
#include "miosix.h"
#include "elf_types.h"
#include "prog1.h"

using namespace std;
using namespace miosix;

extern "C" void syscallSupercazzola(int a, int b, int c);

Thread *svcHandler=0;
Thread *blocked=0;

void svcHandlerThread(void *)
{
    for(;;)
    {
        {
            FastInterruptDisableLock dLock;
            blocked=0;
            for(;;)
            {
                if(blocked!=0) break;
                Thread::getCurrentThread()->IRQwait();
                {
                    FastInterruptEnableLock eLock(dLock);
                    Thread::yield();
                }
            }
        }
        
        miosix_private::SyscallParameters sp=blocked->getSyscallParameters();
        if(sp.isValid())
        {
            switch(sp.getSyscallId())
            {
                case 1:
                    iprintf("Exit %d\n",sp.getFirstParameter());
                    break;
                case 2:
                    iprintf("Write called %d %x %d\n",
                            sp.getFirstParameter(),
                            sp.getSecondParameter(),
                            sp.getThirdParameter());
                    write(sp.getFirstParameter(),
                          (const char*)sp.getSecondParameter(),
                          sp.getThirdParameter());
                    blocked->wakeup();
                    break;
            }
        } else {
            iprintf("Unexpected invalid syscall\n");
        }
        sp.invalidate();
    }
}

void ledThread(void *)
{
    for(;;)
    {
        ledOn();
        Thread::sleep(200);
        ledOff();
        Thread::sleep(200);
    }
}

void loadPhdr(const Elf32_Ehdr *elf)
{
    assert(elf->e_phentsize==sizeof(Elf32_Phdr));
    unsigned int base=reinterpret_cast<unsigned int>(elf);
    const Elf32_Phdr *hdr=reinterpret_cast<const Elf32_Phdr*>(base+elf->e_phoff);
    for(int i=0;i<elf->e_phnum;i++,hdr++)
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

int main()
{
    Thread::create(ledThread,STACK_MIN);
    svcHandler=Thread::create(svcHandlerThread,2048);
    
    getchar();
    
    const Elf32_Ehdr *elf=reinterpret_cast<const Elf32_Ehdr*>(main_elf);
    loadPhdr(elf);
    unsigned int base=reinterpret_cast<unsigned int>(main_elf);
    base+=elf->e_entry;
    void (*elfentry)(void*)=reinterpret_cast<void (*)(void*)>(base);
    iprintf("elf base address = %p\n",main_elf);
    iprintf("elf entry is = %p\n",elf->e_entry);
    iprintf("elf entry (absolute) is = %p\n",elfentry);
            
    Thread::create(elfentry,2048);
    for(;;) Thread::sleep(1000);
}
