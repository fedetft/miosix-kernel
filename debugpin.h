

#ifndef DEBUGPIN_H
#define	DEBUGPIN_H

#include "miosix/interfaces/gpio.h"

typedef miosix::Gpio<GPIOA_BASE, 1> debug1;
typedef miosix::Gpio<GPIOA_BASE,15> debug2;

template<typename T>
class HighPin
{
public:
    HighPin()  { T::high(); }
    ~HighPin() { T::low();  }
};

inline void initDebugPins()
{
    debug1::mode(miosix::Mode::OUTPUT);
    debug2::mode(miosix::Mode::OUTPUT);
}

#endif //DEBUGPIN_H
