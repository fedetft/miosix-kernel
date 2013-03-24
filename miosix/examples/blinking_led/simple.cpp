#include <cstdio>
#include <unistd.h>
#include "miosix.h"

using namespace std;
using namespace miosix;

typedef Gpio<GPIOC_BASE,7>  led;

int main()
{
  
  led::mode(Mode::OUTPUT);
  
  for(;;) {
    led::high();
    sleep(1);
    led::low();
    sleep(1);
  }
  
  return 0;
  
}