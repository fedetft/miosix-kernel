##
## Makefile for board stm32f401vc_stm32f4discovery
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
ARCH_INC := arch/chip/stm32f4

## Select linker script
LINKER_SCRIPT := $(BOARD_INC)/stm32_256k+64k_rom.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                  \
arch/drivers/stm32f2_f4_i2c.cpp       \
$(BOARD_INC)/boot.cpp                        \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_STM32F401VC_STM32F4DISCOVERY
CXXFLAGS_BASE += -D_BOARD_STM32F401VC_STM32F4DISCOVERY

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= st-flash --connect-under-reset --reset write \
        $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000
