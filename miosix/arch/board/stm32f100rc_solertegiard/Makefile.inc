##
## Makefile for board stm32f100rc_solertegiard
## Included by Makefile.inc based on the board currently selected
##

## Base directory with header files for this board
ARCH_INC := arch/chip/stm32f1

## Base directory with header files for this board
BOARD_INC := arch/board/stm32f100rc_solertegiard

## Select linker script
LINKER_SCRIPT := $(BOARD_INC)/stm32_256k+24k_rom.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                  \
$(BOARD_INC)/boot.cpp                        \
$(BOARD_INC)/interfaces-impl/bsp.cpp         \
arch/drivers/servo_stm32.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_SOLERTEGIARD
CXXFLAGS_BASE += -D_BOARD_SOLERTEGIARD

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0

## Include the makefile for the board's chip
include $(KPATH)/$(ARCH_INC)/Makefile.inc
