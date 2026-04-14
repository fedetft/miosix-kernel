##
## Makefile for board nrf52840_generic
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
CHIP_INC := arch/chip/nrf52

## Select linker script
LINKER_SCRIPT := ldscripts/miosix-flash-ram.ld
BOARD_LINKER_SCRIPT := unikernel.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
BOARD_SRC :=                                  \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
BOARD_CFLAGS   := -D_BOARD_NRF52840_GENERIC
BOARD_CXXFLAGS := -D_BOARD_NRF52840_GENERIC

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= echo 'make program not supported.'
