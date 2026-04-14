##
## Options for board stm32f103ze_stm3210e-eval
## Included by the board's makefile
##

## Select linker script

## 1) Code in FLASH, stack + heap in internal RAM
#LINKER_SCRIPT := unikernel.ld

## 2) Code in FLASH, IRQ stack, .data, .bss in internal RAM, heap in external RAM
LINKER_SCRIPT := unikernel-xram-heap.ld
XRAM := -D__ENABLE_XRAM

## 3) Code + stack + heap in external RAM, Code runs *very* slow. Works only
##    with a booloader that forwards interrrupts @ 0x68000000
##    (see miosix/_tools/bootloaders for one).
#LINKER_SCRIPT := unikernel-all-in-xram.ld
#XRAM := -D__ENABLE_XRAM -D__CODE_IN_XRAM

## 4) Code in FLASH, Kernel in internal RAM, Process pool in XRAM
#LINKER_SCRIPT := processes-xram.ld
#XRAM := -D__ENABLE_XRAM
