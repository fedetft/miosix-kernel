##
## Options for board stm32f429zi_stm32f4discovery
## Included by the board's makefile
##

## Select linker script
## 1) Code in FLASH, stack + heap in internal RAM
#LINKER_SCRIPT := unikernel.ld

## 2) Code in FLASH, stack + heap in external RAM
#LINKER_SCRIPT := unikernel-xram-8m.ld

## 3) Same as 2), but leaves the upper 2MB of RAM for the LCD framebuffers
LINKER_SCRIPT := unikernel-xram-6m.ld

## 4) Same as 1) but with the process pool
#LINKER_SCRIPT := processes.ld

## 5) Code in FLASH, kernel in internal RAM, process pool in XRAM, except for
## upper 2M that are reserved for the LCD framebuffers
#LINKER_SCRIPT := processes-xram-6m.ld

## Do not uncomment this even if you don't use a linker script that requires
## it, as it is used for the LCD framebuffer.
XRAM := -D__ENABLE_XRAM
