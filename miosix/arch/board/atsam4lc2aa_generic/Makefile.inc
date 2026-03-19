##
## Makefile for board atsam4lc2aa_generic
## Included by Makefile.inc based on the board currently selected
##

## Base directory with header files for this board
ARCH_INC := arch/chip/atsam4l

## Base directory with header files for this board
BOARD_INC := arch/board/atsam4lc2aa_generic

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                  \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_ATSAM4LC2AA_GENERIC
CXXFLAGS_BASE += -D_BOARD_ATSAM4LC2AA_GENERIC

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
## BUG: flashes ok but requires powercycle
PROG_BIN = $(if $(ROMFS_DIR),image.bin,main.bin)
PROG ?= openocd \
        -f '$(KPATH)/$(BOARD_INC)/openocd.cfg' \
        -c 'program $(PROG_BIN) 0x4000 reset'  \
        -c shutdown

## Include the makefile for the board's chip
include $(KPATH)/$(ARCH_INC)/Makefile.inc
