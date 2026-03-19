##
## Makefile for board stm32f767zi_nucleo
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
ARCH_INC := arch/chip/stm32f7

## The stm32f767 has the double precision FPU, so we will build for m7
CPU := -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16

## Base directory with header files for this board
BOARD_INC := arch/board/stm32f767zi_nucleo

## Select linker script
LINKER_SCRIPT := $(BOARD_INC)/stm32_2m+384k_ram.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                           \
$(BOARD_INC)/boot.cpp                 \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_STM32F767ZI_NUCLEO
CXXFLAGS_BASE += -D_BOARD_STM32F767ZI_NUCLEO

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
