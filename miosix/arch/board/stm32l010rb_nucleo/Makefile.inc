##
## Makefile for board stm32l010rb_nucleo
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
CHIP_INC := arch/chip/stm32l0

## Select linker script
LINKER_SCRIPT := $(BOARD_INC)/stm32_128k+20k_rom.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
BOARD_SRC :=                             \
$(BOARD_INC)/boot.cpp                   \
$(BOARD_INC)/interfaces-impl/delays.cpp \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
BOARD_CFLAGS   := -DBOARD_STM32L010RB_NUCLEO
BOARD_CXXFLAGS := -DBOARD_STM32L010RB_NUCLEO

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= st-flash --connect-under-reset --reset write \
        $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000
