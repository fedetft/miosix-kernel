##
## Options for board stm32l476rg_nucleo
## Included by the board's makefile
##

## Select linker script
# Option 1: use the 96KB ram2 for the heap, no processes
LINKER_SCRIPT := unikernel.ld

# Option 1: use the 96KB ram2 for the process pool
#LINKER_SCRIPT := processes.ld
