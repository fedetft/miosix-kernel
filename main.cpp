
#include <cstdio>
#include <cstdlib>
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
        if(led2::value()) led2::low(); else led2::high();
        tick+=period;
        Thread::setPriority(Priority(tick)); //Change deadline
        Thread::sleepUntil(tick);
    }
}

int main()
{
    //iprintf("Hello world, write your application here\n");
    led1::mode(Mode::OUTPUT);
    led2::mode(Mode::OUTPUT);

    Thread::create(blinkThread,STACK_MIN);
    const int period=static_cast<int>(TICK_FREQ*0.05);
    long long tick=getTick();
    for(;;)
    {
        if(led1::value()) led1::low(); else led1::high();
        tick+=period;
        Thread::setPriority(Priority(tick)); //Change deadline
        Thread::sleepUntil(tick);
    }
}
