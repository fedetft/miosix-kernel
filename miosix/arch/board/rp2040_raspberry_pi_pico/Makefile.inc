##
## Makefile for board rp2040_raspberry_pi_pico
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
ARCH_INC := arch/chip/rp2040

## Select linker script
#LINKER_SCRIPT := already selected in board options

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                                 \
$(ARCH_INC)/rp2040_boot.cpp                                 \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -DBOARD_RP2040_RASPBERRY_PI_PICO -D$(BOARD_VARIANT)
CXXFLAGS_BASE += -DBOARD_RP2040_RASPBERRY_PI_PICO -D$(BOARD_VARIANT)

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= picotool load -x $(if $(ROMFS_DIR), image.bin, main.bin)
