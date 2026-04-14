##
## Options for board stm32f469ni_stm32f469i-disco
## Included by the board's makefile
##

## Select linker script
## 1) Code in FLASH, stack + heap in internal RAM
#LINKER_SCRIPT := unikernel.ld

## 2) Code in FLASH, stack + heap in external RAM
#LINKER_SCRIPT := unikernel-xram-16m.ld

## 3) Same as 2), but leaves the upper 4MB of RAM for the LCD framebuffers
LINKER_SCRIPT := unikernel-xram-12m.ld

## 4) Code in FLASH, kernel in internal RAM, process pool in external RAM
#LINKER_SCRIPT := processes-xram.ld

## 5) Code in FLASH, kernel and process pool in external RAM
#LINKER_SCRIPT := processes-kernel-xram.ld

## Uncommenting __ENABLE_XRAM enables the initialization of the external
## 16MB SDRAM memory. Do not uncomment this even if you don't use a linker
## script that requires it, as it is used for the LCD framebuffer.
XRAM := -D__ENABLE_XRAM
