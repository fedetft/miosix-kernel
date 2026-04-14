##
## Options for board stm32l476rg_nucleo
## Included by the board's makefile
##

## Select linker scripts
# Option 1: use the 96KB ram2 for the heap, no processes
LINKER_SCRIPT := ldscripts/miosix-flash-2ram-separate-heap.ld
BOARD_LINKER_SCRIPT := unikernel.ld

# Option 1: use the 96KB ram2 for the process pool
#LINKER_SCRIPT := ldscripts/miosix-flash-2ram-separate-pool.ld
#BOARD_LINKER_SCRIPT := processes.ld
