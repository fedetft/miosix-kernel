##
## Makefile for board stm32l4r9zi_sensortile
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
CHIP_INC := arch/chip/stm32l4

## Select linker script
#LINKER_SCRIPT := already selected in board options

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
BOARD_SRC :=                                 \
$(BOARD_INC)/boot.cpp                        \
$(BOARD_INC)/interfaces-impl/bsp.cpp         \
arch/drivers/sdmmc/stm32l4_sd.cpp

## Add a #define to allow querying board name
BOARD_CFLAGS   := -D_BOARD_STM32L4R9ZI_SENSORTILE
BOARD_CXXFLAGS := -D_BOARD_STM32L4R9ZI_SENSORTILE

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= st-flash --connect-under-reset --reset write \
        $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000
