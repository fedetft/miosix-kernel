##
## Options for board stm32f429zi_skyward_anakin
## Included by the board's makefile
##

## Select linker scripts
## 1) Code in FLASH, stack + heap in internal RAM
#LINKER_SCRIPT := ldscripts/miosix-flash-3ram-separate-heap-sgm.ld
#BOARD_LINKER_SCRIPT := unikernel-sgm.ld

## 2) Code in FLASH, stack + heap in external RAM
LINKER_SCRIPT := ldscripts/miosix-flash-3ram-separate-heap-sgm.ld
BOARD_LINKER_SCRIPT := unikernel-xram-sgm.ld
XRAM := -D__ENABLE_XRAM
