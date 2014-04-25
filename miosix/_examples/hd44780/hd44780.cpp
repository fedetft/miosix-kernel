
#include <miosix.h>
#include <util/lcd44780.h>

using namespace miosix;

#ifndef _BOARD_STM32F4DISCOVERY
#warning "You may need to choose different GPIOs that are free in your board"
#endif

typedef Gpio<GPIOB_BASE,12> d4;
typedef Gpio<GPIOB_BASE,13> d5;
typedef Gpio<GPIOB_BASE,14> d6;
typedef Gpio<GPIOB_BASE,15> d7;
typedef Gpio<GPIOC_BASE,1> rs;
typedef Gpio<GPIOC_BASE,2> e;

int main()
{
    //First 6 parameters are GPIOs, then there are the number of lines
    //of the display, and number of characters per line
    Lcd44780 display(rs::getPin(),e::getPin(),d4::getPin(),
                     d5::getPin(),d6::getPin(),d7::getPin(),2,16);
    display.clear();
    display.go(0,0);
    display.printf("Miosix + HD44780");
    for(int i=0;;i++)
    {
        Thread::sleep(1000);
        display.go(0,1);
        display.printf("i=%04d",i);
    }
}
