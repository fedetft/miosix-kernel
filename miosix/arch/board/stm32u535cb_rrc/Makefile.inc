##
## Makefile for board stm32u535cb_rrc
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
ARCH_INC := arch/cortexM33_stm32u5/common

## Base directory with header files for this board
BOARD_INC := arch/cortexM33_stm32u5/stm32u535cb_rrc

## Select linker script
LINKER_SCRIPT_PATH := $(BOARD_INC)
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)/stm32_128k+256k_rom.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                  \
$(BOARD_INC)/boot.cpp                        \
$(BOARD_INC)/interfaces-impl/bsp.cpp
# arch/common/drivers/stm32_hardware_rng.cpp   \

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_STM32U535CB_RRC
CXXFLAGS_BASE += -D_BOARD_STM32U535CB_RRC

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D \
        $(if $(ROMFS_DIR), image.bin, main.bin)

## Include the makefile for the board's chip
include $(ARCH_INC)/Makefile.inc
