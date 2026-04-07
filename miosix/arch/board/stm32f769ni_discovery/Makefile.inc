##
## Makefile for board stm32f769ni_discovery
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
CHIP_INC := arch/chip/stm32f7

## The stm32f769 has the double precision FPU, so we will build for m7
CPU := -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16

## Select linker script
LINKER_SCRIPT := $(BOARD_INC)/stm32_2m+16m_xram.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
BOARD_SRC :=                           \
$(BOARD_INC)/boot.cpp                 \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
BOARD_CFLAGS   := -D_BOARD_STM32F769NI_DISCO
BOARD_CXXFLAGS := -D_BOARD_STM32F769NI_DISCO

## Enables the initialization of the external 16MB SDRAM memory
XRAM := -D__ENABLE_XRAM

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= st-flash --connect-under-reset --reset write \
        $(if $(ROMFS_DIR), image.bin, main.bin) 0x08000000
