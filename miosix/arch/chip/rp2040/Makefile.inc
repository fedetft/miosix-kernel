##
## Makefile for chip rp2040
## Included by all board makefiles that use this chip
##

## Select compiler
PREFIX := arm-miosix-eabi
## This architecture supports processes
POSTLD := mx-postlinker

CPU_INC := arch/cpu/armv6m
## Select appropriate compiler flags for both ASM/C/C++/linker
CPU_FLAGS := -mcpu=cortex-m0plus -mthumb
CHIP_CFLAGS   := -D_CHIP_RP2040
CHIP_CXXFLAGS := -D_CHIP_RP2040

## Select architecture specific files
## These are the files in arch/<arch name>/common
CHIP_SRC :=                                              \
arch/cpu/armv6m/interfaces-impl/cpu.cpp                  \
arch/cpu/common/cortexMx_interrupts.cpp                  \
arch/cpu/common/cortexMx_mpu.cpp                         \
arch/cpu/common/cortexMx_userspace.cpp                   \
arch/drivers/os_timer/rp2040_os_timer.cpp                \
arch/drivers/sleep/cortexMx_sleep.cpp                    \
arch/drivers/serial/arm_pl011_serial.cpp                 \
arch/drivers/serial/rp2040_serial.cpp                    \
arch/drivers/spi/arm_pl022_spi.cpp                       \
arch/drivers/spi/rp2040_spi.cpp                          \
arch/drivers/rp2040_dma.cpp                              \
$(CHIP_INC)/interfaces-impl/delays.cpp                   \
$(CHIP_INC)/interfaces-impl/smp.cpp                      \
$(CHIP_INC)/interfaces-impl/atomic_ops_smp_impl.cpp      \
$(CHIP_INC)/interfaces-impl/poweroff.cpp                 \
arch/CMSIS/Device/RaspberryPi/RP2040/Source/system_RP2040.c
