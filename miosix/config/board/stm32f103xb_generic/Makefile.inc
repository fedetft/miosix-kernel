##
## Options for board stm32f103xb_generic
## Included by the board's makefile
##

## Select linker scripts, stm32f103c8 has 64K, stm32f103cb has 128K
LINKER_SCRIPT := ldscripts/miosix-flash-ram.ld
#BOARD_LINKER_SCRIPT := unikernel-64k+20k.ld
BOARD_LINKER_SCRIPT := unikernel-128k+20k.ld
