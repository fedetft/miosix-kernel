##
## Options for board stm32h723zg_nucleo
## Included by the board's makefile
##

## Linker script type, there are three options
## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
##    the most common choice, available for all microcontrollers
## 2) Code in FLASH, stack + heap in external RAM (file *m_xram.ld)
##    You must uncomment -D__ENABLE_XRAM below in this case.
LINKER_SCRIPT_PATH := arch/cortexM7_stm32h7/stm32h723zg_nucleo/
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_rom.ld
