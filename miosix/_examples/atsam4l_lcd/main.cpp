
#include <cstdio>
#include "miosix.h"
#include "drivers/lcd.h"

using namespace std;
using namespace miosix;

int main()
{
    start32kHzOscillator();

    //Configure GPIO of LCD, PA8 to PA23 = 4 COM and 12 SEG
    for(int i = 8; i <= 23; i++)
    {
        GpioPin pin(GPIOA_BASE,i);
        pin.mode(Mode::ALTERNATE);
        pin.alternateFunction('F');
    }

    initLcd(12);

//     enableBlink();
    for(int i = 0; ; i++)
    {
        int t = i;
        beginUpdate();
        for(int j = 0; j < 6 ; j++, t /= 10) setDigit(j, digitTbl[t % 10]);
        endUpdate();
        Thread::sleep(10);
    }
}
