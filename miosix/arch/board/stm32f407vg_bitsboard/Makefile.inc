##
## Makefile for board stm32f407vg_bitsboard
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
CHIP_INC := arch/chip/stm32f4

## Select linker script
LINKER_SCRIPT := $(BOARD_INC)/stm32_1m+192k_rom.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                  \
arch/drivers/stm32_hardware_rng.cpp   \
$(BOARD_INC)/boot.cpp                        \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_BITSBOARD
CXXFLAGS_BASE += -D_BOARD_BITSBOARD

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= st-flash --connect-under-reset --reset write \
        $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000make
