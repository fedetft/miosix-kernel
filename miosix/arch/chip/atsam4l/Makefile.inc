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
CPU_FLAGS := -mcpu=cortex-m4 -mthumb
CHIP_CFLAGS   := -D_CHIP_ATSAM4L
CHIP_CXXFLAGS := -D_CHIP_ATSAM4L

## Select architecture specific files
## These are the files in arch/<arch name>/common
CHIP_SRC :=                                              \
arch/cpu/armv7m/interfaces-impl/cpu.cpp                  \
arch/cpu/common/cortexMx_interrupts.cpp                  \
arch/drivers/mpu/cortexMx_mpu.cpp                        \
arch/cpu/common/cortexMx_userspace.cpp                   \
arch/drivers/os_timer/atsam4l_os_timer.cpp               \
arch/drivers/sleep/cortexMx_sleep.cpp                    \
arch/drivers/serial/serial_atsam4l.cpp                   \
arch/drivers/clock/atsam4l_clock.cpp                     \
arch/drivers/atsam4l_lcd.cpp                             \
$(CHIP_INC)/interfaces-impl/sleep.cpp                    \
arch/drivers/gpio/atsam4l_gpio.cpp                       \
$(CHIP_INC)/interfaces-impl/delays.cpp                   \
$(CHIP_INC)/interfaces-impl/poweroff.cpp
