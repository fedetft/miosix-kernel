##
## Makefile for board stm32f207zg_nucleo
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
CHIP_INC := arch/chip/stm32f2

## Select linker script
LINKER_SCRIPT := ldscripts/miosix-flash-ram.ld
BOARD_LINKER_SCRIPT := unikernel.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
BOARD_SRC :=                            \
arch/drivers/sdmmc/sd_stm32f2_f4_f7.cpp \
$(BOARD_INC)/boot.cpp                   \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
BOARD_CFLAGS   := -D_BOARD_STM32F207ZG_NUCLEO
BOARD_CXXFLAGS := -D_BOARD_STM32F207ZG_NUCLEO

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= st-flash --connect-under-reset --reset write \
        $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000
