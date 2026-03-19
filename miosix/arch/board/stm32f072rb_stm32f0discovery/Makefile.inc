##
## Makefile for board stm32f072rb_stm32f0discovery
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
ARCH_INC := arch/cortexM0_stm32f0/common

## Base directory with header files for this board
BOARD_INC := arch/cortexM0_stm32f0/stm32f072rb_stm32f0discovery

## Select linker script
LINKER_SCRIPT := $(BOARD_INC)/stm32_128k+16k_rom.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                   \
$(BOARD_INC)/boot.cpp                         \
$(BOARD_INC)/interfaces-impl/bsp.cpp          \
arch/common/os_timer/stm32_32bit_os_timer.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_STM32F072RB_DISCO
CXXFLAGS_BASE += -D_BOARD_STM32F072RB_DISCO

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
