// This tool is meant to test delays using an oscilloscope connected to a GPIO
// of the chip.

#include <cstdio>
#include "miosix.h"

using namespace miosix;

using out=Gpio<PA,1>; //Select a free GPIO depending on the board

void tdus(int n)
{
    GlobalIrqLock dLock;
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
    GlobalIrqLock dLock;
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
//    using mco = Gpio<PA,8>;
//    mco::mode(Mode::ALTERNATE);
//    RCC->CFGR |= (0x7<<24);
//    //STM32f?-specific: enable PLL freq to be output on PA8
//    using mco = Gpio<PA,8>;
//    mco::speed(Speed::_100MHz);
//    mco::mode(Mode::ALTERNATE);
//    mco::alternateFunction(0);
//    RCC->CFGR |= (0x3<<21);
//    //STM32L1-specific: enable SYSCLK/2 freq to be output on PA8
//    using mco = Gpio<PA,8>;
//    mco::speed(Speed::VERY_HIGH);
//    mco::mode(Mode::ALTERNATE);
//    mco::alternateFunction(0);
//    RCC->CFGR &= ~(RCC_CFGR_MCOSEL | RCC_CFGR_MCOPRE);
//    RCC->CFGR |= (1 << RCC_CFGR_MCOPRE_Pos) | RCC_CFGR_MCOSEL_SYSCLK;
//    //ATSAM4L-specific: output RCFAST clock on PA2
//    SCIF->SCIF_GCCTRL[0].SCIF_GCCTRL = SCIF_GCCTRL_OSCSEL(5) | SCIF_GCCTRL_CEN;
//    using gclk0 = Gpio<PA,2>;
//    gclk0::mode(Mode::ALTERNATE);
//    gclk0::alternateFunction('A');
//    //EFM32-specific: output HFCLK/2 clock on PA2
//    using clkOut0 = Gpio<PA,2>;
//    clkOut0::mode(Mode::OUTPUT);
//    CMU->CTRL |= CMU_CTRL_CLKOUTSEL0_HFCLK2;
//    CMU->ROUTE |= CMU_ROUTE_CLKOUT0PEN;
//    //Raspberry Pi Pico-specific: output clock on GPIO 21 divided by 1000
//    using clkOut = Gpio<P0,21>;
//    clocks_hw->clk[clk_gpout0].ctrl=
//        (CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS<<CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB)
//        | CLOCKS_CLK_GPOUT0_CTRL_ENABLE_BITS;
//    clocks_hw->clk[clk_gpout0].div=1000<<CLOCKS_CLK_GPOUT0_DIV_INT_LSB;
    int n;
    out::mode(Mode::OUTPUT);
    iprintf("Delay test\nEnter value in us\n");
    iscanf("%d",&n);
    if(n>=1000) tdms(n/1000); else tdus(n);
}
