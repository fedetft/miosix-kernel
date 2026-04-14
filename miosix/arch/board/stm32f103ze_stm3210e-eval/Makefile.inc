##
## Makefile for board stm32f103ze_stm3210e-eval
## Included by Makefile.inc based on the board currently selected
##

## Base directory with header files for this board
CHIP_INC := arch/chip/stm32f1

## Select linker script
#LINKER_SCRIPT := already selected in board options

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
BOARD_SRC :=                                \
$(BOARD_INC)/boot.cpp                       \
arch/drivers/sdmmc/sd_stm32f1.cpp           \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
BOARD_CFLAGS   := -D_BOARD_STM3210E_EVAL
BOARD_CXXFLAGS := -D_BOARD_STM3210E_EVAL

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
ifeq ($(LINKER_SCRIPT),unikernel-all-in-xram.ld)
    PROG ?= $(KPATH)/_tools/bootloaders/stm32/pc_loader/pc_loader \
            /dev/ttyUSB0 $(if $(ROMFS_DIR), image.bin, main.bin)
else
    PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0
endif
