##
## Options for board stm32f429zi_skyward_homeone
## Included by the board's makefile
##

## Select linker script
## 1) Code in FLASH, stack + heap in internal RAM
#LINKER_SCRIPT := unikernel.ld

## 2) Code in FLASH, stack + heap in external RAM
LINKER_SCRIPT := unikernel-xram.ld
XRAM := -D__ENABLE_XRAM
