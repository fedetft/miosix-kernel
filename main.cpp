#include <miosix.h>
using namespace miosix;
int main()
{
    //iprintf("Hello world, write your application here\n");

    for(;;)
    {
        ledOn();
        Thread::sleep(1000);
        ledOff();
        Thread::sleep(1000);
    }
}
    
