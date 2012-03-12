
#include <cstdio>
#include "miosix.h"
#include "ELF.h"
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

int main()
{
    Thread::create(ledThread,STACK_MIN);
    svcHandler=Thread::create(svcHandlerThread,2048);
    
    const elf32_ehdr *elf=reinterpret_cast<const elf32_ehdr*>(main_elf);
    unsigned int base=reinterpret_cast<unsigned int>(main_elf);
    base+=+elf->e_entry;
    void (*elfentry)(void*)=reinterpret_cast<void (*)(void*)>(base);
    iprintf("elf base address = %p\n",main_elf);
    iprintf("elf entry is = %p\n",elf->e_entry);
    iprintf("elf entry (absolute) is = %p\n",elfentry);
            
    Thread::create(elfentry,2048);
    for(;;) Thread::sleep(1000);
}
