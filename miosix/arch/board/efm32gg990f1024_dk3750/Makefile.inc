##
## Makefile for board efm32gg990f1024_dk3750
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
CHIP_INC := arch/chip/efm32gg

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
BOARD_SRC :=                             \
$(BOARD_INC)/interfaces-impl/bsp.cpp     \
arch/drivers/os_timer/efm32_os_timer.cpp

## Add a #define to allow querying board name
BOARD_CFLAGS   := -D_BOARD_EFM32GG990F1024_DK3750
BOARD_CXXFLAGS := -D_BOARD_EFM32GG990F1024_DK3750

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG_BIN = $(if $(ROMFS_DIR),image.bin,main.bin)
PROG ?= openocd \
        -f '$(KPATH)/$(BOARD_INC)/openocd.cfg' \
        -c 'program $(PROG_BIN) 0x0 verify reset' \
        -c shutdown
