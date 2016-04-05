
#include <cstdio>
#include "miosix.h"

using namespace std;
using namespace miosix;

bool status=false;
Thread *a=NULL;
Thread *b=NULL;

void f(void *argv)
{
    for(;;)
    {
        delayMs(250);

        {
            PauseKernelLock dLock;
            delayMs(2);
        }
        delayMs(250);

        Thread::sleep(2);

        delayMs(250);
    }
}

int main()
{
    a=Thread::getCurrentThread();
    b=Thread::create(f,STACK_MIN);
    for(;;) delayMs(500);
}
