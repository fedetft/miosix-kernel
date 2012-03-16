
#include <cstdio>
#include <cassert>
#include "miosix.h"
#include "elf_program.h"
#include "prog2.h"

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
    
    getchar();
    
    ElfProgram prog(reinterpret_cast<const unsigned int*>(main_elf),main_elf_len);
    ProcessImage pi;
    pi.load(prog);

    void *(*entry)(void*)=reinterpret_cast<void *(*)(void*)>(prog.getEntryPoint());
    iprintf("elf base address = 0x%x\n",prog.getElfBase());
    iprintf("elf entry (rebased) = 0x%x\n",prog.getEntryPoint());
    iprintf("base pointer = %p\n",pi.getProcessBasePointer());
    
    Thread::createWithGotBase(entry,2048,1,0,0,pi.getProcessBasePointer());
    for(;;) Thread::sleep(1000);
}
