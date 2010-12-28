
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "miosix.h"

using namespace std;
using namespace miosix;

typedef Gpio<GPIOF_BASE,6> led1;
typedef Gpio<GPIOF_BASE,9> led2;

void blinkThread(void *argv)
{
    const int period=static_cast<int>(TICK_FREQ*0.03);
    long long tick=getTick();
    for(;;)
    {
        long long prevTick=tick;
        tick+=period;
        Thread::setPriority(Priority(tick)); //Change deadline
        Thread::sleepUntil(prevTick); //Make sure the task is run periodically
        if(led2::value()) led2::low(); else led2::high();
        delayMs(14);
        if(getTick()>tick) iprintf("Deadline missed (A)\n");
    }
}

int main()
{
    //iprintf("Hello world, write your application here\n");
    led1::mode(Mode::OUTPUT);
    led2::mode(Mode::OUTPUT);

    Thread::create(blinkThread,2048);
    const int period=static_cast<int>(TICK_FREQ*0.05);
    long long tick=getTick();
    for(;;)
    {
        long long prevTick=tick;
        tick+=period;
        Thread::setPriority(Priority(tick)); //Change deadline
        Thread::sleepUntil(prevTick); //Make sure the task is run periodically
        if(led1::value()) led1::low(); else led1::high();
        delayMs(24);
        if(getTick()>tick) iprintf("Deadline missed (B)\n");
    }
}
