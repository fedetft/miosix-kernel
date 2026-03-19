#pragma once

#define STM32H503xx
#include "CMSIS/Device/ST/STM32H5xx/Include/stm32h5xx.h"
#include "CMSIS/Include/core_cm33.h"
#include "CMSIS/Device/ST/STM32H5xx/Include/system_stm32h5xx.h"

//RCC_SYNC is a Miosix-defined primitive for synchronizing the CPU with the
//STM32 RCC (Reset and Clock Control) peripheral after modifying peripheral
//clocks/resets. This is necessary on almost all stm32s starting from stm32f2xx,
//as observed experimentally across a variety of microcontrollers and also
//according to the errata and more recent reference manuals. To be extra safe
//it is defined to a __DSB() on all stm32s (even those that according to the
//documentation don't need it).
//On stm32h503 RCC synchronization is required per the reference manual
//(RM0492 section 10.4.18).
#define RCC_SYNC() __DSB()

//Peripheral interrupt start from 0 and the last one is 133, so there are 134
#define MIOSIX_NUM_PERIPHERAL_IRQ 134
