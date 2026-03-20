##
## Makefile for board lpc2138_miosix_board
## Included by Makefile.inc based on the board currently selected
##

## Base directory with header files for this board
ARCH_INC := arch/chip/lpc2000

## Select linker script
LINKER_SCRIPT := arch/board/lpc2138_miosix_board/lpc2138_500k+32k_rom.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                  \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_MIOSIX_BOARD
CXXFLAGS_BASE += -D_BOARD_MIOSIX_BOARD
