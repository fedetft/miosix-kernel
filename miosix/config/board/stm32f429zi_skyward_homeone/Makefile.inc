##
## Options for board stm32f429zi_skyward_homeone
## Included by the board's makefile
##

## Linker script type, there are three options
## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
##    the most common choice, available for all microcontrollers
## 2) Code in FLASH, stack + heap in external RAM (file *8m_xram.ld)
##    You must uncomment -D__ENABLE_XRAM below in this case.
LINKER_SCRIPT_PATH := arch/board/stm32f429zi_skyward_homeone/
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+256k_rom.ld
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+8m_xram.ld

## Uncommenting __ENABLE_XRAM enables the initialization of the external
## 8MB SDRAM memory.
XRAM := -D__ENABLE_XRAM
