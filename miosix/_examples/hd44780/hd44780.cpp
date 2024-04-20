
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

// Custom characters:
//   0      1      2      3      4      5      6      7
// ..X..  .....  .....  .....  .....  .....  .....  .....
// ..X..  ....X  .....  .....  .....  .....  .....  X....
// ..X..  ...X.  .....  .....  .....  .....  .....  .X...
// ..X..  ..X..  ..XXX  ..X..  ..X..  ..X..  XXX..  ..X..
// .....  .....  .....  ...X.  ..X..  .X...  .....  .....
// .....  .....  .....  ....X  ..X..  X....  .....  .....
// .....  .....  .....  .....  ..X..  .....  .....  .....
// .....  .....  .....  .....  .....  .....  .....  .....
static const unsigned char clockFont[8][8] = {
    { 0x04,0x04,0x04,0x04,0x00,0x00,0x00,0x00 },
    { 0x00,0x01,0x02,0x04,0x00,0x00,0x00,0x00 },
    { 0x00,0x00,0x00,0x07,0x00,0x00,0x00,0x00 }, 
    { 0x00,0x00,0x00,0x04,0x02,0x01,0x00,0x00 }, 
    { 0x00,0x00,0x00,0x04,0x04,0x04,0x04,0x00 }, 
    { 0x00,0x00,0x00,0x04,0x08,0x10,0x00,0x00 }, 
    { 0x00,0x00,0x00,0x1C,0x00,0x00,0x00,0x00 }, 
    { 0x00,0x10,0x08,0x04,0x00,0x00,0x00,0x00 }
};

int main()
{
    //First 6 parameters are GPIOs, then there are the number of lines
    //of the display, and number of characters per line
    Lcd44780 display(rs::getPin(),e::getPin(),d4::getPin(),
                     d5::getPin(),d6::getPin(),d7::getPin(),2,16);
    display.clear();
    for(int i=0; i<8; i++) display.setCgram(i, clockFont[i]);
    display.go(0,0);
    display.printf("Miosix + HD44780");
    for(int i=0;;i++)
    {
        display.go(2,1);
        display.printf("[ %c i=%04d ]",i%8,i);
        Thread::sleep(1000);
    }
}
