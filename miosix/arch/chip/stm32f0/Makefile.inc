##
## Makefile for chip stm32f0
## Included by all board makefiles that use this chip
##

## Select compiler
PREFIX := arm-miosix-eabi-
## This architecture does not support processes
#POSTLD :=

CPU_INC := arch/cpu/armv6m
## Select appropriate compiler flags for both ASM/C/C++/linker
CPU := -mcpu=cortex-m0 -mthumb
AFLAGS_BASE   := $(CPU)
CFLAGS_BASE   += -D_ARCH_CORTEXM0_STM32F0 $(CPU) \
                 $(OPT_OPTIMIZATION)
CXXFLAGS_BASE += -D_ARCH_CORTEXM0_STM32F0 $(CPU) \
                 $(OPT_OPTIMIZATION) $(OPT_EXCEPT)
LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map        \
                 -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT) \
                 $(OPT_OPTIMIZATION) -nostdlib

## Select architecture specific files
## These are the files in arch/<arch name>/common

ARCH_SRC +=                                              \
arch/cpu/armv6m/interfaces-impl/cpu.cpp                  \
arch/cpu/common/cortexMx_interrupts.cpp                  \
arch/common/sleep/cortexMx_sleep.cpp                     \
arch/common/drivers/stm32_gpio.cpp                       \
arch/common/drivers/stm32f7_serial.cpp                   \
arch/common/drivers/stm32_serial_common.cpp              \
$(ARCH_INC)/interfaces-impl/delays.cpp                   \
$(ARCH_INC)/interfaces-impl/poweroff.cpp                 \
arch/common/CMSIS/Device/ST/STM32F0xx/Source/Templates/system_stm32f0xx.cpp
