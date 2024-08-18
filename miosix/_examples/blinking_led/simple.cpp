#include <cstdio>

#include "miosix.h"

using namespace miosix;

int main()
{
    while(true)
    {
        ledOn();
        printf("Led on\n");
        sleep(1);

        ledOff();
        printf("Led off\n");
        sleep(1);
    }

    return 0;
}
