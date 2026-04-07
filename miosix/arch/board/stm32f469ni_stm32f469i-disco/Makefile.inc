##
## Makefile for board stm32f469ni_stm32f469i-disco
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
CHIP_INC := arch/chip/stm32f4

## Select linker script
#LINKER_SCRIPT := already selected in board options

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                  \
arch/drivers/stm32f2_f4_i2c.cpp       \
arch/drivers/stm32_hardware_rng.cpp   \
$(BOARD_INC)/boot.cpp                        \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_STM32F469NI_STM32F469I_DISCO
CXXFLAGS_BASE += -D_BOARD_STM32F469NI_STM32F469I_DISCO

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= st-flash --connect-under-reset --reset write \
        $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000
