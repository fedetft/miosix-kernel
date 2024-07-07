//These tests are meant to help coding os timer drivers

#include <cstdio>
#include "miosix.h"

using namespace std;
using namespace miosix;

void smallsleepTest()
{
    // Check for os_timer race conditions by sleeping close to the wakeup time
    // test passes if no lockups occur
    using pin = Gpio<GPIOB_BASE,1>; //TODO: change pin to one available
    pin::mode(Mode::OUTPUT);
    const int span=2;
    for(;;)
    {
        for(int i=0;i<span*1000;i++)
        {
            long long t=getTime()+1000000ll*span;
            pin::high();
            delayUs(i);
            pin::low();
            Thread::nanoSleepUntil(t);
        }
        putchar('.'); fflush(stdout);
        for(int i=span*1000;i>0;i--)
        {
            long long t=getTime()+1000000ll*span;
            pin::high();
            delayUs(i);
            pin::low();
            Thread::nanoSleepUntil(t);
        }
        putchar('.'); fflush(stdout);
    }
}

void lagTest()
{
    // Measure sleep wakup lag both when sleeping and deep sleeping
    const long long period=1000000000ll;
    long long t=getTime();
    for(;;)
    {
        t+=period;
        Thread::nanoSleepUntil(t);
        long long t2=getTime();
        iprintf("deep sleep %lld\n",t2-t);
        {
            DeepSleepLock sl;
            t+=period;
            Thread::nanoSleepUntil(t);
            long long t2=getTime();
            iprintf("     sleep %lld\n",t2-t);
        }
    }
}

int main()
{
//     smallsleepTest();
    lagTest();
}
