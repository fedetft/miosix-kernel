##
## Options for board stm32f207ig_stm3220g-eval
## Included by the board's makefile
##

## Select linker script
## 1) Code in FLASH, stack + heap in internal RAM
#LINKER_SCRIPT := ldscripts/miosix-flash-ram.ld
#BOARD_LINKER_SCRIPT := unikernel.ld

## 2) Code in FLASH, IRQ stack, .data, .bss in internal RAM, heap in external RAM
LINKER_SCRIPT := ldscripts/miosix-flash-2ram-separate-heap.ld
BOARD_LINKER_SCRIPT := unikernel-xram-heap.ld
XRAM := -D__ENABLE_XRAM

## 3) Code in FLASH, IRQ stack in internal RAM, heap, data and bss in external RAM
#LINKER_SCRIPT := ldscripts/miosix-flash-2ram-separate-irqstack.ld
#BOARD_LINKER_SCRIPT := unikernel-xram.ld
#XRAM := -D__ENABLE_XRAM

## 4) Code + stack + heap in external RAM, Code runs *very* slow. Works only
##    with a booloader that forwards interrrupts @ 0x64000000
##    (see miosix/_tools/bootloaders for one).
#LINKER_SCRIPT := ldscripts/miosix-all-in-xram.ld
#BOARD_LINKER_SCRIPT := unikernel-all-in-xram.ld
#XRAM := -D__ENABLE_XRAM -D__CODE_IN_XRAM

## 5) Code in FLASH, kernel and process pool in internal RAM
#LINKER_SCRIPT := ldscripts/miosix-flash-ram.ld
#BOARD_LINKER_SCRIPT := processes.ld

## 6) Code in FLASH, Kernel in internal RAM, Process pool in XRAM
#LINKER_SCRIPT := ldscripts/miosix-flash-2ram-separate-pool.ld
#BOARD_LINKER_SCRIPT := processes-xram.ld
#XRAM := -D__ENABLE_XRAM

## 4) Same as 3) but space has been reserved for a process pool, allowing
##    to configure the kernel with "#define WITH_PROCESSES"
## 5) Code in FLASH, kernel stack+heap in internal RAM, ELF pool in Flash
##    and process pool in external RAM.
###LINKER_SCRIPT_PATH := arch/board/stm32f207ig_stm3220g-eval/
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_rom.ld
###LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_xram.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_all_in_xram.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_xram_processes.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_xram-data-bss-heap.ld
