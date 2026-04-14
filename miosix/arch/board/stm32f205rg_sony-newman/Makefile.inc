##
## Makefile for board stm32f205rg_sony-newman
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
CHIP_INC := arch/chip/stm32f2

## Select linker script
LINKER_SCRIPT := ldscripts/miosix-flash-ram.ld
BOARD_LINKER_SCRIPT := unikernel.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
BOARD_SRC :=                         \
arch/drivers/stm32f2_f4_i2c.cpp      \
$(BOARD_INC)/boot.cpp                \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
BOARD_CFLAGS   := -D_BOARD_SONY_NEWMAN
BOARD_CXXFLAGS := -D_BOARD_SONY_NEWMAN

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
## The magic.bin is somewhat used by the bootloader to detect a good fw
PROG ?= perl -e 'print "\xe7\x91\x11\xc0"' > magic.bin; \
        dfu-util -d 0fce:f0fa -a 0 -i 0 -s 0x08040000 -D main.bin;  \
        dfu-util -d 0fce:f0fa -a 0 -i 0 -s 0x080ffffc -D magic.bin; \
        rm magic.bin
