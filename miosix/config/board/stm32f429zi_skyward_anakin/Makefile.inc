##
## Options for board stm32f429zi_skyward_anakin
## Included by the board's makefile
##

## Select linker script
## 1) Code in FLASH, stack + heap in internal RAM
#LINKER_SCRIPT := unikernel-sgm.ld

## 2) Code in FLASH, stack + heap in external RAM
LINKER_SCRIPT := unikernel-xram-sgm.ld
XRAM := -D__ENABLE_XRAM
