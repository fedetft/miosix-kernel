
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
//    //STM32-specific: enable PLL freq to be output on PA8
//    typedef Gpio<GPIOA_BASE,8> mco;
//    mco::speed(Speed::_100MHz);
//    mco::mode(Mode::ALTERNATE);
//    mco::alternateFunction(0);
//    RCC->CFGR |= (0x3<<21);
//    //ATSAM4L-specific: output RCFAST clock on PA2
//    SCIF->SCIF_GCCTRL[0].SCIF_GCCTRL = SCIF_GCCTRL_OSCSEL(5) | SCIF_GCCTRL_CEN;
//    using gclk0 = Gpio<GPIOA_BASE,2>;
//    gclk0::mode(Mode::ALTERNATE);
//    gclk0::alternateFunction('A');
    int n;
    out::mode(Mode::OUTPUT);
    iprintf("Delay test\nEnter value in us\n");
    scanf("%d",&n);
    if(n>=1000) tdms(n/1000); else tdus(n);
}
