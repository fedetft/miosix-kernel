##
## Options for board stm32f100xb_generic
## Included by the board's makefile
##

# stm32f100c8 has 64K, stm32f100cb has 128K
LINKER_SCRIPT := ldscripts/miosix-flash-ram.ld
#BOARD_LINKER_SCRIPT := unikernel-64k+8k.ld
BOARD_LINKER_SCRIPT := unikernel-128k+8k.ld
