##
## Options for board stm32f429zi_oledboard2
## Included by the board's makefile
##

## Linker script type, there are three options
## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
##    the most common choice, available for all microcontrollers
## 2) Code in FLASH, stack + heap in external RAM (file *8m_xram.ld)
##    You must uncomment -D__ENABLE_XRAM below in this case.
## 3) Code in FLASH, stack + heap in external RAM (file *6m_xram.ld)
##    Same as above, but leaves the upper 2MB of RAM for the LCD.
LINKER_SCRIPT_PATH := arch/cortexM4_stm32f4/stm32f429zi_oledboard2/
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+256k_rom.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+8m_xram.ld
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+6m_xram.ld

## Uncommenting __ENABLE_XRAM enables the initialization of the external
## 8MB SDRAM memory. Do not uncomment this even if you don't use a linker
## script that requires it, as it is used for the LCD framebuffer.
XRAM := -D__ENABLE_XRAM
