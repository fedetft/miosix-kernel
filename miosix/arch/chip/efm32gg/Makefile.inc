##
## Makefile for chip efm32gg
## Included by all board makefiles that use this chip
##

## Select compiler
PREFIX := arm-miosix-eabi-
## This architecture supports processes
POSTLD := mx-postlinker

CPU_INC := arch/cpu/armv7m
## Select appropriate compiler flags for both ASM/C/C++/linker
CPU := -mcpu=cortex-m3 -mthumb
CHIP_AFLAGS   := $(CPU)
CHIP_CFLAGS   := -D_CHIP_EFM32GG $(CPU)
CHIP_CXXFLAGS := -D_CHIP_EFM32GG $(CPU)
CHIP_LFLAGS   := $(CPU)

## Select architecture specific files
## These are the files in arch/<arch name>/common
CHIP_SRC :=                                              \
arch/cpu/armv7m/interfaces-impl/cpu.cpp                  \
arch/cpu/common/cortexMx_interrupts.cpp                  \
arch/drivers/mpu/cortexMx_mpu.cpp                        \
arch/cpu/common/cortexMx_userspace.cpp                   \
arch/drivers/sleep/cortexMx_sleep.cpp                    \
arch/drivers/gpio/efm32_gpio.cpp                         \
arch/drivers/serial/efm32_serial.cpp                     \
arch/drivers/efm32_adc.cpp                               \
$(CHIP_INC)/interfaces-impl/delays.cpp                   \
$(CHIP_INC)/interfaces-impl/poweroff.cpp
