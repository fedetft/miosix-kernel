##
## Makefile for chip atsam4l
## Included by all board makefiles that use this chip
##

## Select compiler
PREFIX := arm-miosix-eabi-
## This architecture supports processes
POSTLD := mx-postlinker

CPU_INC := arch/cpu/armv7m
## Select appropriate compiler flags for both ASM/C/C++/linker
CPU := -mcpu=cortex-m4 -mthumb
AFLAGS_BASE   := $(CPU)
CFLAGS_BASE   += -D_ARCH_CORTEXM4_ATSAM4L $(CPU) $(OPT_OPTIMIZATION)
CXXFLAGS_BASE += -D_ARCH_CORTEXM4_ATSAM4L $(CPU) $(OPT_OPTIMIZATION) \
                 $(OPT_EXCEPT)
LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map         \
                 -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT)  \
                 $(OPT_OPTIMIZATION) -nostdlib

## Select architecture specific files
## These are the files in arch/<arch name>/common
ARCH_SRC +=                                              \
arch/cpu/armv7m/interfaces-impl/cpu.cpp                  \
arch/cpu/common/cortexMx_interrupts.cpp                  \
arch/common/mpu/cortexMx_mpu.cpp                         \
arch/cpu/common/cortexMx_userspace.cpp                   \
arch/common/os_timer/atsam4l_os_timer.cpp                \
arch/common/sleep/cortexMx_sleep.cpp                     \
arch/common/drivers/serial_atsam4l.cpp                   \
$(ARCH_INC)/drivers/clock.cpp                            \
$(ARCH_INC)/drivers/lcd.cpp                              \
$(ARCH_INC)/interfaces-impl/sleep.cpp                    \
$(ARCH_INC)/interfaces-impl/gpio.cpp                     \
$(ARCH_INC)/interfaces-impl/delays.cpp                   \
$(ARCH_INC)/interfaces-impl/poweroff.cpp
