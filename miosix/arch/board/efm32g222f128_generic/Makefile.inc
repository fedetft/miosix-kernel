##
## Makefile for board efm32g222f128_generic
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
ARCH_INC := arch/cortexM3_efm32g/common

## Base directory with header files for this board
BOARD_INC := arch/cortexM3_efm32g/efm32g222f128_generic

## Select linker script
LINKER_SCRIPT := $(BOARD_INC)/efm32_128k+16k_rom_bootloader.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                                 \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= echo 'make program not supported.'

## Include the makefile for the board's chip
include $(ARCH_INC)/Makefile.inc
