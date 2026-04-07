##
## Makefile for chip stm32f1
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
CHIP_CFLAGS   := -D_CHIP_STM32F1 $(XRAM) $(CPU)
CHIP_CXXFLAGS := -D_CHIP_STM32F1 $(XRAM) $(CPU)
CHIP_LFLAGS   := $(CPU)

## Select architecture specific files
## These are the files in arch/<arch name>/common
CHIP_SRC :=                                           \
arch/cpu/armv7m/interfaces-impl/cpu.cpp               \
arch/cpu/common/cortexMx_interrupts.cpp               \
arch/cpu/common/cortexMx_userspace.cpp                \
arch/drivers/os_timer/stm32_16bit_os_timer.cpp        \
arch/drivers/os_timer/stm32_rtc_os_timer.cpp          \
arch/drivers/sleep/cortexMx_sleep.cpp                 \
arch/drivers/gpio/stm32f1_gpio.cpp                    \
arch/drivers/serial/stm32f1_f2_f4_serial.cpp          \
arch/drivers/serial/stm32_serial_common.cpp           \
arch/drivers/dcc.cpp                                  \
$(CHIP_INC)/interfaces-impl/delays.cpp                \
$(CHIP_INC)/interfaces-impl/sleep.cpp                 \
$(CHIP_INC)/interfaces-impl/poweroff.cpp              \
arch/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.cpp
