##
## Makefile for chip stm32l0
## Included by all board makefiles that use this chip
##

## Select compiler
PREFIX := arm-miosix-eabi-
## This architecture does not support processes
#POSTLD :=

CPU_INC := arch/cpu/armv6m
## Select appropriate compiler flags for both ASM/C/C++/linker
CPU := -mcpu=cortex-m0plus -mthumb
AFLAGS_BASE   := $(CPU)
CFLAGS_BASE   += -D_ARCH_CORTEXM0PLUS_STM32L0 $(CPU) \
                 $(OPT_OPTIMIZATION)
CXXFLAGS_BASE += -D_ARCH_CORTEXM0PLUS_STM32L0 $(CPU) \
                 $(OPT_OPTIMIZATION) $(OPT_EXCEPT)
LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map        \
                 -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT) \
                 $(OPT_OPTIMIZATION) -nostdlib

## Select architecture specific files
## These are the files in arch/<arch name>/common

ARCH_SRC +=                                              \
arch/cpu/armv6m/interfaces-impl/cpu.cpp                  \
arch/cpu/common/cortexMx_interrupts.cpp                  \
arch/drivers/os_timer/stm32_16bit_os_timer.cpp            \
arch/drivers/sleep/cortexMx_sleep.cpp                     \
arch/drivers/gpio/stm32_gpio.cpp                       \
arch/drivers/serial/stm32f7_serial.cpp                   \
arch/drivers/serial/stm32_serial_common.cpp              \
$(ARCH_INC)/interfaces-impl/poweroff.cpp                 \
arch/CMSIS/Device/ST/STM32L0xx/Source/Templates/system_stm32l0xx.cpp
