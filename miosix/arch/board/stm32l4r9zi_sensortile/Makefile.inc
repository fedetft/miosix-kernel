##
## Makefile for board stm32l4r9zi_sensortile
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
ARCH_INC := arch/chip/stm32l4

## Base directory with header files for this board
BOARD_INC := arch/board/stm32l4r9zi_sensortile

## Select linker script
#LINKER_SCRIPT := already selected in board options

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                  \
$(BOARD_INC)/boot.cpp                        \
$(BOARD_INC)/interfaces-impl/bsp.cpp         \
arch/drivers/sdmmc/sd_stm32l4.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_STM32L4R9ZI_SENSORTILE
CXXFLAGS_BASE += -D_BOARD_STM32L4R9ZI_SENSORTILE

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= st-flash --connect-under-reset --reset write \
        $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000

## Include the makefile for the board's chip
include $(KPATH)/$(ARCH_INC)/Makefile.inc
