

#ifndef DEBUGPIN_H
#define	DEBUGPIN_H

#include "miosix/interfaces/gpio.h"

#ifdef _BOARD_STM32F4DISCOVERY
typedef miosix::Gpio<GPIOA_BASE, 1> debug1;
typedef miosix::Gpio<GPIOA_BASE, 15> debug2;
#elif _BOARD_STM32VLDISCOVERY
typedef miosix::Gpio<GPIOB_BASE, 0> debug1;
typedef miosix::Gpio<GPIOB_BASE, 1> debug2;
#elif _BOARD_WANDSTEM
typedef miosix::Gpio<GPIOC_BASE, 2> debug1; // on the board: gpio7
typedef miosix::Gpio<GPIOC_BASE, 7> debug2; // on the board: gpio9
#endif

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
