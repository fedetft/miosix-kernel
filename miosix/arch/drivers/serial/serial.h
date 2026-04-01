#ifdef _CHIP_LPC2000
#include "serial_lpc2000.h"
#elif defined(_CHIP_STM32F1) || defined(_CHIP_STM32F2) \
   || defined(_CHIP_STM32F4) || defined(_CHIP_STM32L1)
#include "stm32f1_f2_f4_serial.h"
#elif defined(_CHIP_STM32F7) || defined(_CHIP_STM32L0) \
   || defined(_CHIP_STM32L4) || defined(_CHIP_STM32F3) \
   || defined(_CHIP_STM32F0) || defined(_CHIP_STM32H7) \
   || defined(_CHIP_STM32H5) || defined(_CHIP_STM32U5)
#include "stm32f7_serial.h"
#elif defined(_CHIP_EFM32GG) || defined(_CHIP_EFM32G)
#include "efm32_serial.h"
#elif defined(_CHIP_ATSAM4L)
#include "serial_atsam4l.h"
#elif defined(_CHIP_RP2040)
#include "rp2040_serial.h"
#else
#error "Unknown chip"
#endif
