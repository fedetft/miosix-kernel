##
## Makefile for chip lpc2000
## Included by all board makefiles that use this chip
##

## Select compiler
PREFIX := arm-miosix-eabi-
## This architecture does not support processes
#POSTLD :=

CPU_INC := arch/cpu/armv4
## Select appropriate compiler flags for both ASM/C/C++/linker
CPU := -mcpu=arm7tdmi
AFLAGS_BASE   := $(CPU)
CFLAGS_BASE   += -D_CHIP_LPC2000 $(CPU) $(OPT_OPTIMIZATION)
CXXFLAGS_BASE += -D_CHIP_LPC2000 $(CPU) $(OPT_OPTIMIZATION) $(OPT_EXCEPT)
LFLAGS_BASE   := $(CPU) -Wl,--gc-sections,-Map,main.map        \
                 -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT) \
                  $(OPT_OPTIMIZATION) -nostdlib

## Select architecture specific files
## These are the files in arch/<arch name>/common
ARCH_SRC +=                                    \
arch/cpu/armv4/interfaces-impl/cpu.cpp         \
arch/cpu/armv4/interfaces-impl/interrupts.cpp  \
arch/drivers/os_timer/lpc2000_os_timer.cpp     \
arch/drivers/sleep/lpc2000_sleep.cpp           \
arch/drivers/serial/serial_lpc2000.cpp         \
arch/drivers/sdmmc/sd_lpc2000.cpp              \
arch/drivers/clock/lpc2000_clock.cpp           \
$(ARCH_INC)/interfaces-impl/delays.cpp         \
$(ARCH_INC)/interfaces-impl/poweroff.cpp

## Select programmer command line
## This is the program that is invoked when the user types 'make program'
## The command must provide a way to program the board, or print an error
## message saying that 'make program' is not supported for that board.
PROG ?= lpc21isp -verify main.hex /dev/ttyUSB0 115200 14746
