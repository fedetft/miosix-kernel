##
## Makefile for board stm32f207ze_als_camboard
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
ARCH_INC := arch/cortexM3_stm32f2/common

## Base directory with header files for this board
BOARD_INC := arch/cortexM3_stm32f2/stm32f207ze_als_camboard

## Select linker script
LINKER_SCRIPT := $(BOARD_INC)/stm32_1m+128k_rom.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                  \
$(BOARD_INC)/boot.cpp                        \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_ALS_CAMBOARD
CXXFLAGS_BASE += -D_BOARD_ALS_CAMBOARD

## XRAM is always enabled in this board
XRAM += -D__ENABLE_XRAM

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0

## Include the makefile for the board's chip
include $(ARCH_INC)/Makefile.inc
