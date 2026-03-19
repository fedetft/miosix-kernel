##
## Options for board stm32f103xb_generic
## Included by the board's makefile
##

# stm32f103c8 has 64K, stm32f103cb has 128K
LINKER_SCRIPT_PATH := arch/board/stm32f103xb_generic/
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_64k+20k_rom.ld
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_128k+20k_rom.ld
