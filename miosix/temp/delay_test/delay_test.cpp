
#include <stdio.h>
#include "miosix.h"

using namespace miosix;

typedef Gpio<GPIOA_BASE,1> out; //Select a free GPIO depending on the board

void tdus(int n)
{
    InterruptDisableLock dLock;
    for(;;)
    {
        out::high();
        delayUs(n);
        out::low();
        delayUs(n);
    }
}

void tdms(int n)
{
    InterruptDisableLock dLock;
    for(;;)
    {
        out::high();
        delayMs(n);
        out::low();
        delayMs(n);
    }
}

int main()
{
    int n;
    out::mode(Mode::OUTPUT);
    iprintf("Delay test\nEnter value in us\n");
    scanf("%d",&n);
    if(n>=1000) tdms(n/1000); else tdus(n);
}
