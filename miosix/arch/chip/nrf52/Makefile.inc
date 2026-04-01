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
AFLAGS_BASE   := $(CPU)
CFLAGS_BASE   += -D_CHIP_NRF52 $(CPU) $(OPT_OPTIMIZATION)
CXXFLAGS_BASE += -D_CHIP_NRF52 $(CPU) $(OPT_OPTIMIZATION) $(OPT_EXCEPT)
LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map         \
                 -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT)  \
                 $(OPT_OPTIMIZATION) -nostdlib

## Select architecture specific files
## These are the files in arch/<arch name>/common
ARCH_SRC +=                                              \
arch/cpu/armv7m/interfaces-impl/cpu.cpp                  \
arch/cpu/common/cortexMx_interrupts.cpp                  \
arch/drivers/mpu/cortexMx_mpu.cpp                        \
arch/cpu/common/cortexMx_userspace.cpp                   \
arch/drivers/os_timer/nrf52_os_timer.cpp                 \
arch/drivers/sleep/cortexMx_sleep.cpp                    \
arch/drivers/dcc.cpp                                     \
arch/drivers/clock/nrf52_clock.cpp                       \
$(ARCH_INC)/interfaces-impl/delays.cpp                   \
$(ARCH_INC)/interfaces-impl/poweroff.cpp                 \
arch/drivers/gpio/nrf52_gpio.cpp
