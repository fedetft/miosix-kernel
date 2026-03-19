##
## Makefile for board stm32f100rb_stm32vldiscovery
## Included by Makefile.inc based on the board currently selected
##

## Base directory with header files for this board
ARCH_INC := arch/cortexM3_stm32f1/common

## Base directory with header files for this board
BOARD_INC := arch/cortexM3_stm32f1/stm32f100rb_stm32vldiscovery

## Select linker script
LINKER_SCRIPT := $(BOARD_INC)/stm32_128k+8k_rom.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                  \
$(BOARD_INC)/interfaces-impl/bsp.cpp         \
$(BOARD_INC)/boot.cpp                        \
arch/common/drivers/stm32_rtc.cpp            \
arch/common/drivers/servo_stm32.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_STM32VLDISCOVERY
CXXFLAGS_BASE += -D_BOARD_STM32VLDISCOVERY

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0

## Include the makefile for the board's chip
include $(ARCH_INC)/Makefile.inc
