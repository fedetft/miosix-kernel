##
## Makefile for board stm32f100cb_tempsensor
## Included by Makefile.inc based on the board currently selected
##

## Base directory with header files for this board
CHIP_INC := arch/chip/stm32f1

## Select linker script
LINKER_SCRIPT := $(BOARD_INC)/stm32_127k+8k_rom.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                  \
$(BOARD_INC)/boot.cpp                        \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_TEMPSENSOR
CXXFLAGS_BASE += -D_BOARD_TEMPSENSOR

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0
