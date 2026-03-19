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
AFLAGS_BASE   := $(CPU)
CFLAGS_BASE   += -D_ARCH_CORTEXM3_STM32F1 $(XRAM) $(CPU) \
                 $(OPT_OPTIMIZATION)
CXXFLAGS_BASE += -D_ARCH_CORTEXM3_STM32F1 $(XRAM) $(CPU) \
                 $(OPT_OPTIMIZATION) $(OPT_EXCEPT)
LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map        \
                 -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT) \
                 $(OPT_OPTIMIZATION) -nostdlib

## Select architecture specific files
## These are the files in arch/<arch name>/common
ARCH_SRC +=                                           \
arch/cpu/armv7m/interfaces-impl/cpu.cpp               \
arch/cpu/common/cortexMx_interrupts.cpp               \
arch/cpu/common/cortexMx_userspace.cpp                \
arch/common/os_timer/stm32_16bit_os_timer.cpp         \
arch/common/os_timer/stm32_rtc_os_timer.cpp           \
arch/common/sleep/cortexMx_sleep.cpp                  \
arch/common/drivers/stm32f1_gpio.cpp                  \
arch/common/drivers/stm32f1_f2_f4_serial.cpp          \
arch/common/drivers/stm32_serial_common.cpp           \
arch/common/drivers/dcc.cpp                           \
$(ARCH_INC)/interfaces-impl/delays.cpp                \
$(ARCH_INC)/interfaces-impl/sleep.cpp                 \
$(ARCH_INC)/interfaces-impl/poweroff.cpp              \
arch/common/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.cpp
