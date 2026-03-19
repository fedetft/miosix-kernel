##
## Options for board stm32h753xi_eval
## Included by the board's makefile
##

## Linker script type, there are three options
## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
##    the most common choice, available for all microcontrollers
## 2) Code in FLASH, stack + heap in external RAM (file *m_xram.ld)
##    You must uncomment -D__ENABLE_XRAM below in this case.
LINKER_SCRIPT_PATH := arch/board/stm32h753xi_eval/
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+512k_rom.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+32m_xram.ld
