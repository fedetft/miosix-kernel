
#include <cstdio>
#include "miosix.h"
#include "kernel/process.h"
#include "app_template/prog3.h"

using namespace std;
using namespace miosix;

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
    
    getchar();
    
    ElfProgram prog(reinterpret_cast<const unsigned int*>(main_elf),main_elf_len);
    Process::create(prog);
    for(;;) Thread::sleep(1000);
}
