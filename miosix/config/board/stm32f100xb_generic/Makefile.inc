##
## Options for board stm32f100xb_generic
## Included by the board's makefile
##

# stm32f100c8 has 64K, stm32f100cb has 128K
LINKER_SCRIPT_PATH := arch/board/stm32f100xb_generic/
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_64k+8k_rom.ld
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_128k+8k_rom.ld
