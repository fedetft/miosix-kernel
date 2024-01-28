#include <cstdio>
#include "miosix.h"
#include "kernel/logging.h"
#include "interfaces/arch_registers.h"

using namespace std;
using namespace miosix;

// This program attempts to test context switching behavior.
// The main thread is busy doing IO work while a secondary thread continuously
// checks for its registers not to change.
// If register corruption is detected in the second thread, the program hangs
// up.

extern "C" void fail()
{
    __disable_irq();
    for(;;);
}

void *otherThread(void *unused)
{
    asm("   cpsid i\n"
        "   ldr r0,=0x55555555\n"
        "   mov r1,r0\n"
        "   mov r2,r0\n"
        "   mov r3,r0\n"
        "   mov r4,r0\n"
        "   mov r5,r0\n"
        "   mov r6,r0\n"
        "   mov r7,r0\n"
        "   mov r8,r0\n"
        "   mov r9,r0\n"
        "   mov r10,r0\n"
        "   mov r11,r0\n"
        "   mov r12,r0\n"
        "   cpsie i\n"
        "1: cmp r0, r1\n    bne 1f\n"
        "   cmp r1, r0\n    bne 1f\n"
        "   cmp r2, r0\n    bne 1f\n"
        "   cmp r3, r0\n    bne 1f\n"
        "   cmp r4, r0\n    bne 1f\n"
        "   cmp r5, r0\n    bne 1f\n"
        "   cmp r6, r0\n    bne 1f\n"
        "   cmp r7, r0\n    bne 1f\n"
        "   cmp r8, r0\n    bne 1f\n"
        "   cmp r9, r0\n    bne 1f\n"
        "   cmp r10,r0\n    bne 1f\n"
        "   cmp r11,r0\n    bne 1f\n"
        "   cmp r12,r0\n    bne 1f\n"
        "   b 1b\n"
        "1: bl fail\n":::
        "r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12");
    return nullptr;
}

int main()
{
    Thread::create(otherThread, 1024);
    for (;;)
    {
        iprintf("blah blah blah blah blah blah blah blah blah blah blah "
            "blah blah blah blah blah blah blah blah blah blah blah "
            "blah blah blah blah blah blah blah blah blah blah blah\n");
    }
}
