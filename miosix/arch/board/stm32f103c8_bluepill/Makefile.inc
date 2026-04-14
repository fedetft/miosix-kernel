##
## Makefile for board stm32f103c8_bluepill
## Included by Makefile.inc based on the board currently selected
##

## Base directory with header files for this board
CHIP_INC := arch/chip/stm32f1

## Select linker script
LINKER_SCRIPT := ldscripts/miosix-flash-ram.ld
BOARD_LINKER_SCRIPT := unikernel.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
BOARD_SRC :=                         \
$(BOARD_INC)/boot.cpp                \
$(BOARD_INC)/interfaces-impl/bsp.cpp \
arch/drivers/servo_stm32.cpp

## Add a #define to allow querying board name
BOARD_CFLAGS   := -D_BOARD_STM32F103C8_BLUEPILL
BOARD_CXXFLAGS := -D_BOARD_STM32F103C8_BLUEPILL

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0
