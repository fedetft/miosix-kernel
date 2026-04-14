##
## Options for board stm32f429zi_stm32f4discovery
## Included by the board's makefile
##

## Select linker scripts
## 1) Code in FLASH, stack + heap in internal RAM
#LINKER_SCRIPT := ldscripts/miosix-flash-2ram-separate-heap.ld
#BOARD_LINKER_SCRIPT := unikernel.ld

## 2) Code in FLASH, stack + heap in external RAM
#LINKER_SCRIPT := ldscripts/miosix-flash-2ram-separate-irqstack.ld
#BOARD_LINKER_SCRIPT := unikernel-xram-8m.ld

## 3) Same as 2), but leaves the upper 2MB of RAM for the LCD framebuffers
LINKER_SCRIPT := ldscripts/miosix-flash-2ram-separate-irqstack.ld
BOARD_LINKER_SCRIPT := unikernel-xram-6m.ld

## 4) Same as 1) but with the process pool
#LINKER_SCRIPT := ldscripts/miosix-flash-2ram-separate-heap.ld
#BOARD_LINKER_SCRIPT := processes.ld

## TODO Add processes + XRAM options

## Do not uncomment this even if you don't use a linker script that requires
## it, as it is used for the LCD framebuffer.
XRAM := -D__ENABLE_XRAM
