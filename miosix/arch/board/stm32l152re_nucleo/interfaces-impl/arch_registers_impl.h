#pragma once

#define STM32L152xE
#include "CMSIS/Device/ST/STM32L1xx/Include/stm32l1xx.h"
#include "CMSIS/Include/core_cm3.h"
#include "CMSIS/Device/ST/STM32L1xx/Include/system_stm32l1xx.h"

//RCC_SYNC is a Miosix-defined primitive for synchronizing the CPU with the
//STM32 RCC (Reset and Clock Control) peripheral after modifying peripheral
//clocks/resets. This is necessary on almost all stm32s starting from stm32f2xx,
//as observed experimentally across a variety of microcontrollers and also
//according to the errata and more recent reference manuals. To be extra safe
//it is defined to a __DSB() on all stm32s (even those that according to the
//documentation don't need it).
//On stm32l151,l152 RCC synchronization is required per the device errata
//(ES0224 section 2.2.4).
#define RCC_SYNC() __DSB()

//Peripheral interrupt start from 0 and the last one is 56, so there are 57
#define MIOSIX_NUM_PERIPHERAL_IRQ 57
