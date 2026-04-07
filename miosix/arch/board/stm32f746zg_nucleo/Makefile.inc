##
## Makefile for board stm32f746zg_nucleo
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
CHIP_INC := arch/chip/stm32f7

## The stm32f746 lacks the double precision FPU, so we will build for m4
CPU_FLAGS := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
BOARD_SRC :=                                  \
$(BOARD_INC)/boot.cpp                        \
arch/drivers/stm32_hardware_rng.cpp   \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
BOARD_CFLAGS   := -D_BOARD_STM32F746ZG_NUCLEO
BOARD_CXXFLAGS := -D_BOARD_STM32F746ZG_NUCLEO

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= st-flash --connect-under-reset --reset write \
        $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000
