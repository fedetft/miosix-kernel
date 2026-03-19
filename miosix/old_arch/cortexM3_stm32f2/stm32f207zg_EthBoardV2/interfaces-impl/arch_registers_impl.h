#pragma once

#define STM32F207xx
#include "CMSIS/Device/ST/STM32F2xx/Include/stm32f2xx.h"
#include "CMSIS/Include/core_cm3.h"
#include "CMSIS/Device/ST/STM32F2xx/Include/system_stm32f2xx.h"

//RCC_SYNC is a Miosix-defined primitive for synchronizing the CPU with the
//STM32 RCC (Reset and Clock Control) peripheral after modifying peripheral
//clocks/resets. This is necessary on almost all stm32s starting from stm32f2xx,
//as observed experimentally across a variety of microcontrollers and also
//according to the errata and more recent reference manuals. To be extra safe
//it is defined to a __DSB() on all stm32s (even those that according to the
//documentation don't need it).
//On stm32f20x,f21x RCC synchronization is required per the device errata
//(ES0005 section 2.1.11).
#define RCC_SYNC() __DSB()

//Peripheral interrupt start from 0 and the last one is 80, so there are 81
#define MIOSIX_NUM_PERIPHERAL_IRQ 81
