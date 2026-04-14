##
## Options for board stm32f415vg_st25dvdiscovery
## Included by the board's makefile
##

## Select linker scripts
LINKER_SCRIPT := ldscripts/miosix-flash-2ram-separate-heap.ld
BOARD_LINKER_SCRIPT := unikernel.ld
#BOARD_LINKER_SCRIPT := processes.ld
