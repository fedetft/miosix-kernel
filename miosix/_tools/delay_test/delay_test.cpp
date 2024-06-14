// This tool is meant to test delays using an oscilloscope connected to a GPIO
// of the chip.

#include <cstdio>
#include "miosix.h"

using namespace miosix;

using out=Gpio<GPIOA_BASE,1>; //Select a free GPIO depending on the board

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
//    //STM32f1-specific: enable PLL freq to be output on PA8
//    using mco = Gpio<GPIOA_BASE,8>;
//    mco::mode(Mode::ALTERNATE);
//    RCC->CFGR |= (0x7<<24);
//    //STM32f?-specific: enable PLL freq to be output on PA8
//    using mco = Gpio<GPIOA_BASE,8>;
//    mco::speed(Speed::_100MHz);
//    mco::mode(Mode::ALTERNATE);
//    mco::alternateFunction(0);
//    RCC->CFGR |= (0x3<<21);
//    //ATSAM4L-specific: output RCFAST clock on PA2
//    SCIF->SCIF_GCCTRL[0].SCIF_GCCTRL = SCIF_GCCTRL_OSCSEL(5) | SCIF_GCCTRL_CEN;
//    using gclk0 = Gpio<GPIOA_BASE,2>;
//    gclk0::mode(Mode::ALTERNATE);
//    gclk0::alternateFunction('A');
//    //EFM32-specific: output HFCLK/2 clock on PA2
//    using clkOut0 = Gpio<GPIOA_BASE,2>;
//    clkOut0::mode(Mode::OUTPUT);
//    CMU->CTRL |= CMU_CTRL_CLKOUTSEL0_HFCLK2;
//    CMU->ROUTE |= CMU_ROUTE_CLKOUT0PEN;
    int n;
    out::mode(Mode::OUTPUT);
    iprintf("Delay test\nEnter value in us\n");
    iscanf("%d",&n);
    if(n>=1000) tdms(n/1000); else tdus(n);
}
