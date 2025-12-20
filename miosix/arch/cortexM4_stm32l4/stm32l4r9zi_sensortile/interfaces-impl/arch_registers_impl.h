#pragma once

//Always include stm32l4xx.h before core_cm4.h, there's some nasty dependency
#define STM32L4R9xx
#include "CMSIS/Device/ST/STM32L4xx/Include/stm32l4xx.h"
#include "CMSIS/Include/core_cm4.h"
#include "CMSIS/Device/ST/STM32L4xx/Include/system_stm32l4xx.h"

//RCC_SYNC is a Miosix-defined primitive for synchronizing the CPU with the
//STM32 RCC (Reset and Clock Control) peripheral after modifying peripheral
//clocks/resets. This is necessary on almost all stm32s starting from stm32f2xx,
//as observed experimentally across a variety of microcontrollers and also
//according to the errata and more recent reference manuals. To be extra safe
//it is defined to a __DSB() on all stm32s (even those that according to the
//documentation don't need it).
//On stm32l4, RCC synchronization is required per the reference manual
//(RM0432 section 6.2.18).
#define RCC_SYNC() __DSB()

//Peripheral interrupt start from 0 and the last one is 94, so there are 95
#define MIOSIX_NUM_PERIPHERAL_IRQ 95
