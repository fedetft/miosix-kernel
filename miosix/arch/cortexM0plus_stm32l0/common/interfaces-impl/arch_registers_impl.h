
#ifndef ARCH_REGISTERS_IMPL_H
#define	ARCH_REGISTERS_IMPL_H

//Always include stm32xxxx.h before core_xxx.h, there's some nasty dependency
#include "CMSIS/Device/ST/STM32L0xx/Include/stm32l0xx.h"
#include "CMSIS/Include/core_cm0plus.h"
#include "CMSIS/Device/ST/STM32L0xx/Include/system_stm32l0xx.h"

#define RCC_SYNC() // It appears it doesn't need to be implemented. TODO: CHECK

#endif	//ARCH_REGISTERS_IMPL_H
