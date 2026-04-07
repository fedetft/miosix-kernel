##
## Makefile for chip nrf52
## Included by all board makefiles that use this chip
##

## Select compiler
PREFIX := arm-miosix-eabi-
## This architecture supports processes
POSTLD := mx-postlinker

CPU_INC := arch/cpu/armv7m
## Select appropriate compiler flags for both ASM/C/C++/linker
CPU := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
CHIP_AFLAGS   := $(CPU)
CHIP_CFLAGS   := -D_CHIP_NRF52 $(CPU)
CHIP_CXXFLAGS := -D_CHIP_NRF52 $(CPU)
CHIP_LFLAGS   := $(CPU)

## Select architecture specific files
## These are the files in arch/<arch name>/common
CHIP_SRC :=                                              \
arch/cpu/armv7m/interfaces-impl/cpu.cpp                  \
arch/cpu/common/cortexMx_interrupts.cpp                  \
arch/drivers/mpu/cortexMx_mpu.cpp                        \
arch/cpu/common/cortexMx_userspace.cpp                   \
arch/drivers/os_timer/nrf52_os_timer.cpp                 \
arch/drivers/sleep/cortexMx_sleep.cpp                    \
arch/drivers/dcc.cpp                                     \
arch/drivers/clock/nrf52_clock.cpp                       \
$(CHIP_INC)/interfaces-impl/delays.cpp                   \
$(CHIP_INC)/interfaces-impl/poweroff.cpp                 \
arch/drivers/gpio/nrf52_gpio.cpp
