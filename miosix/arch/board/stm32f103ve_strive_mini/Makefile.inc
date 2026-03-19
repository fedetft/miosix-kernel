##
## Makefile for board stm32f103ve_strive_mini
## Included by Makefile.inc based on the board currently selected
##

## Base directory with header files for this board
ARCH_INC := arch/cortexM3_stm32f1/common

## Base directory with header files for this board
BOARD_INC := arch/cortexM3_stm32f1/stm32f103ve_strive_mini

## Select linker script
LINKER_SCRIPT := $(BOARD_INC)/stm32_512k+64k_rom.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                  \
$(BOARD_INC)/boot.cpp                        \
arch/common/drivers/sd_stm32f1.cpp           \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_STRIVE_MINI
CXXFLAGS_BASE += -D_BOARD_STRIVE_MINI

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= st-flash --connect-under-reset --reset write \
        $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

## Include the makefile for the board's chip
include $(ARCH_INC)/Makefile.inc
