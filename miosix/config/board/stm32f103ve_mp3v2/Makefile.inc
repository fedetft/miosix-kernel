##
## Options for board stm32f103ve_mp3v2
## Included by the board's makefile
##

## Linker script type, there are two options
## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
## 2) Code + stack + heap in internal RAM (file *_ram.ld)
## Note: this board relies on a bootloader for interrupt forwarding in ram
LINKER_SCRIPT_PATH := arch/cortexM3_stm32f1/stm32f103ve_mp3v2/
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_512k+64k_rom.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_512k+64k_ram.ld
