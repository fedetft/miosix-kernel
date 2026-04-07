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
CHIP_AFLAGS   := $(CPU)
CHIP_CFLAGS   := -D_CHIP_STM32F0 $(CPU) \
                 $(OPT_OPTIMIZATION)
CHIP_CXXFLAGS := -D_CHIP_STM32F0 $(CPU) \
                 $(OPT_OPTIMIZATION) $(OPT_EXCEPT)
CHIP_LFLAGS   := $(CPU) -Wl,--gc-sections,-Map,main.map        \
                 -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT) \
                 $(OPT_OPTIMIZATION) -nostdlib

## Select architecture specific files
## These are the files in arch/<arch name>/common

CHIP_SRC :=                                              \
arch/cpu/armv6m/interfaces-impl/cpu.cpp                  \
arch/cpu/common/cortexMx_interrupts.cpp                  \
arch/drivers/sleep/cortexMx_sleep.cpp                    \
arch/drivers/gpio/stm32_gpio.cpp                         \
arch/drivers/serial/stm32f7_serial.cpp                   \
arch/drivers/serial/stm32_serial_common.cpp              \
$(CHIP_INC)/interfaces-impl/delays.cpp                   \
$(CHIP_INC)/interfaces-impl/poweroff.cpp                 \
arch/CMSIS/Device/ST/STM32F0xx/Source/Templates/system_stm32f0xx.cpp
