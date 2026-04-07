##
## Makefile for board nrf52840_generic
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
CHIP_INC := arch/chip/nrf52

## Select linker script
LINKER_SCRIPT := $(BOARD_INC)/nrf_1024k+256k_rom.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                  \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_NRF52840_GENERIC
CXXFLAGS_BASE += -D_BOARD_NRF52840_GENERIC

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= echo 'make program not supported.'
