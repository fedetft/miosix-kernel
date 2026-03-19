#pragma once

// stm32f7xx.h defines a few macros like __ICACHE_PRESENT, __DCACHE_PRESENT and
// includes core_cm7.h. Do not include core_cm7.h before.
#define STM32F765xx
#include "CMSIS/Device/ST/STM32F7xx/Include/stm32f7xx.h"

#if (__ICACHE_PRESENT != 1) || (__DCACHE_PRESENT != 1)
#error "Wrong include order"
#endif

#include "CMSIS/Device/ST/STM32F7xx/Include/system_stm32f7xx.h"

//RCC_SYNC is a Miosix-defined primitive for synchronizing the CPU with the
//STM32 RCC (Reset and Clock Control) peripheral after modifying peripheral
//clocks/resets. This is necessary on almost all stm32s starting from stm32f2xx,
//as observed experimentally across a variety of microcontrollers and also
//according to the errata and more recent reference manuals. To be extra safe
//it is defined to a __DSB() on all stm32s (even those that according to the
//documentation don't need it).
//On stm32f76x,f77x RCC synchronization is required per the reference manual
//(RM0410 section 5.2.12).
#define RCC_SYNC() __DSB()

//Peripheral interrupt start from 0 and the last one is 109, so there are 110
#define MIOSIX_NUM_PERIPHERAL_IRQ 110
