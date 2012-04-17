
#include <cstdio>
#include "miosix.h"
#include "kernel/process.h"
#include "app_template/prog3.h"

using namespace std;
using namespace miosix;

int main()
{
    getchar();
    
    ElfProgram prog(reinterpret_cast<const unsigned int*>(main_elf),main_elf_len);
    Process::create(prog);
    for(;;)
    {
        ledOn();
        Thread::sleep(200);
        ledOff();
        Thread::sleep(200);
    }
}
