##
## Makefile for board stm32f103ve_mp3v2
## Included by Makefile.inc based on the board currently selected
##

## Base directory with header files for this board
CHIP_INC := arch/chip/stm32f1

## Select linker script
#LINKER_SCRIPT := already selected in board options

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                  \
$(BOARD_INC)/boot.cpp                        \
arch/drivers/sdmmc/sd_stm32f1.cpp           \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_MP3V2
CXXFLAGS_BASE += -D_BOARD_MP3V2

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
ifeq ($(LINKER_SCRIPT),$(LINKER_SCRIPT_PATH)stm32_512k+64k_ram.ld)
    PROG ?= mp3v2_bootloader --ram main.bin
else
    PROG ?= mp3v2_bootloader --code $(if $(ROMFS_DIR), image.bin, main.bin)
endif
