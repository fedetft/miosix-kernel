
//Interrupt code is common for all the cortex M cores, so it has been put here

#ifdef _ARCH_ARM7_LPC2000
#include "interrupts_arm7.h"
#elif defined(_ARCH_CORTEXM3_STM32)
#include "interrupts_cortexMx.h"
#elif defined(_ARCH_CORTEXM4_STM32F4)
#include "interrupts_cortexMx.h"
#elif defined(_ARCH_CORTEXM3_STM32F2)
#include "interrupts_cortexMx.h"
#elif defined(_ARCH_CORTEXM3_STM32L1)
#include "interrupts_cortexMx.h"
#else
#error "Unknown arch"
#endif
