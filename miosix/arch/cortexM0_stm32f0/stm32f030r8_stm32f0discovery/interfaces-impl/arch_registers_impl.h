#pragma once

//Always include stm32f0xx.h before core_cm0.h, there's some nasty dependency
#define STM32F030x8
#include "CMSIS/Device/ST/STM32F0xx/Include/stm32f0xx.h"
#include "CMSIS/Include/core_cm0.h"
#include "CMSIS/Device/ST/STM32F0xx/Include/system_stm32f0xx.h"

//RCC_SYNC is a Miosix-defined primitive for synchronizing the CPU with the
//STM32 RCC (Reset and Clock Control) peripheral after modifying peripheral
//clocks/resets. This is necessary on almost all stm32s starting from stm32f2xx,
//as observed experimentally across a variety of microcontrollers and also
//according to the errata and more recent reference manuals. To be extra safe
//it is defined to a __DSB() on all stm32s (even those that according to the
//documentation don't need it).
#define RCC_SYNC() __DSB()

//Peripheral interrupt start from 0 and the last one is 28, so there are 29
#define MIOSIX_NUM_PERIPHERAL_IRQ 29
