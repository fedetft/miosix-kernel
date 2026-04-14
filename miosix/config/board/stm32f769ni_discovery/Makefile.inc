##
## Options for board stm32f769ni_discovery
## Included by the board's makefile
##

## Select linker script

# Option 1: IRQ stack in TCM SRAM, everything else in 16MB XRAM
LINKER_SCRIPT := unikernel-xram.ld
XRAM := -D__ENABLE_XRAM

# Option 2: kernel in internal SRAM, process pool in 16MB XRAM
#LINKER_SCRIPT := processes-xram.ld
#XRAM := -D__ENABLE_XRAM

# Option 3: everything in SRAM, XRAM not used
#LINKER_SCRIPT := unikernel.ld

# Option 4: everything in SRAM+process pool, XRAM not used
#LINKER_SCRIPT := processes.ld
