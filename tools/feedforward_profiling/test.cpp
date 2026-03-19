
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
        status=true;
        Thread::sleep(250);
        status=false;
        delayMs(250);
    }
}

int main()
{
    a=Thread::getCurrentThread();
    b=Thread::create(f,STACK_MIN);
    for(;;) delayMs(500);
}
