##
## Makefile for chip stm32u5
## Included by all board makefiles that use this chip
##

## Select compiler
PREFIX := arm-miosix-eabi-
## This architecture supports processes
POSTLD := mx-postlinker

CPU_INC := arch/cpu/armv7m
## Select appropriate compiler flags for both ASM/C/C++/linker
CPU := -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
AFLAGS_BASE   := $(CPU)
CFLAGS_BASE   += -D_ARCH_CORTEXM33_STM32U5 $(XRAM) \
                 $(CPU) $(OPT_OPTIMIZATION)
CXXFLAGS_BASE += -D_ARCH_CORTEXM33_STM32U5 $(XRAM) \
                 $(CPU) $(OPT_OPTIMIZATION) $(OPT_EXCEPT)
LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map         \
                 -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT)  \
                 $(OPT_OPTIMIZATION) -nostdlib

## Select architecture specific files
## These are the files in arch/<arch name>/common
ARCH_SRC +=                                              \
arch/cpu/armv7m/interfaces-impl/cpu.cpp                  \
arch/cpu/common/cortexMx_interrupts.cpp                  \
arch/drivers/mpu/cortexMx_mpu.cpp                         \
arch/cpu/common/cortexMx_userspace.cpp                   \
arch/drivers/os_timer/stm32_32bit_os_timer.cpp            \
arch/drivers/sleep/cortexMx_sleep.cpp                     \
arch/drivers/cache/cortexMx_cache.cpp                     \
arch/drivers/serial/stm32u5_serial.cpp                   \
arch/drivers/serial/stm32_serial_common.cpp              \
arch/drivers/gpio/stm32_gpio.cpp                       \
arch/drivers/dcc.cpp                              \
$(ARCH_INC)/interfaces-impl/delays.cpp                   \
$(ARCH_INC)/interfaces-impl/poweroff.cpp                 \
arch/drivers/clock/stm32u5_pll.cpp                              \
arch/CMSIS/Device/ST/STM32U5xx/Source/Templates/system_stm32u5xx.cpp
