##
## Options for board stm32f207zg_ethboard_v2
## Included by the board's makefile
##

## XRAM is always enabled on this board

## Select linker script
## 1) Code in FLASH, Kernel in internal RAM, Process pool in XRAM
LINKER_SCRIPT := processes-xram.ld

## 2) Code in FLASH, IRQ stack, .data, .bss in internal RAM, heap in external RAM
#LINKER_SCRIPT := unikernel-xram-heap.ld

## 4) Code + stack + heap in external RAM, Code runs *very* slow. Works only
##    with a booloader that forwards interrrupts @ 0x60000000
##    (see tools/bootloaders for one).
#LINKER_SCRIPT := unikernel-all-in-xram.ld
#XRAM := -D__CODE_IN_XRAM
