##
## Makefile for chip rp2040
## Included by all board makefiles that use this chip
##

## Select compiler
PREFIX := arm-miosix-eabi-
## This architecture supports processes
POSTLD := mx-postlinker

CPU_INC := arch/cpu/armv6m
## Select appropriate compiler flags for both ASM/C/C++/linker
CPU := -mcpu=cortex-m0plus -mthumb
AFLAGS_BASE   := $(CPU)
CFLAGS_BASE   += -D_ARCH_CORTEXM0PLUS_RP2040 $(CPU) \
                 $(OPT_OPTIMIZATION)
CXXFLAGS_BASE += -D_ARCH_CORTEXM0PLUS_RP2040 $(CPU) \
                 $(OPT_OPTIMIZATION) $(OPT_EXCEPT)
LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map        \
                 -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT) \
                 $(OPT_OPTIMIZATION) -nostdlib

## Select architecture specific files
## These are the files in arch/<arch name>/common

ARCH_SRC +=                                              \
arch/cpu/armv6m/interfaces-impl/cpu.cpp                  \
arch/cpu/common/cortexMx_interrupts.cpp                  \
arch/common/mpu/cortexMx_mpu.cpp                         \
arch/cpu/common/cortexMx_userspace.cpp                   \
arch/common/os_timer/rp2040_os_timer.cpp                 \
arch/common/sleep/cortexMx_sleep.cpp                     \
arch/common/drivers/arm_pl011_serial.cpp                 \
arch/common/drivers/rp2040_serial.cpp                    \
arch/common/drivers/arm_pl022_spi.cpp                    \
arch/common/drivers/rp2040_spi.cpp                       \
arch/common/drivers/rp2040_dma.cpp                       \
$(ARCH_INC)/interfaces-impl/delays.cpp                   \
$(ARCH_INC)/interfaces-impl/smp.cpp                      \
$(ARCH_INC)/interfaces-impl/atomic_ops_smp_impl.cpp      \
$(ARCH_INC)/interfaces-impl/poweroff.cpp                 \
arch/common/CMSIS/Device/RaspberryPi/RP2040/Source/system_RP2040.c
