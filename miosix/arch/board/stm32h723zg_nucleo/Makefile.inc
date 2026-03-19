##
## Makefile for board stm32h723zg_nucleo
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
ARCH_INC := arch/cortexM7_stm32h7/common

## Base directory with header files for this board
BOARD_INC := arch/cortexM7_stm32h7/stm32h723zg_nucleo

## Select linker script
#LINKER_SCRIPT := already selected in board options

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                  \
$(BOARD_INC)/boot.cpp           \
arch/common/drivers/stm32_hardware_rng.cpp   \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_STM32H723ZG_NUCLEO
CXXFLAGS_BASE += -D_BOARD_STM32H723ZG_NUCLEO

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
