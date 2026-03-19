
#include "stm32f1_gpio.h"

namespace miosix {

void GpioPin::mode(Mode m)
{
    unsigned char n=getNumber();
    auto ptr=getPortDevice();
    if(n<8)
    {
        ptr->CRL = (ptr->CRL & ~(0xf<<(n*4))) | toUint(m)<<(n*4);
    } else {
        ptr->CRH = (ptr->CRH & ~(0xf<<((n-8)*4))) | toUint(m)<<((n-8)*4);
    }
}

} //namespace miosix
