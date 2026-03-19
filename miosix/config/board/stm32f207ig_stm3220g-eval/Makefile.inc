##
## Options for board stm32f207ig_stm3220g-eval
## Included by the board's makefile
##

## Linker script type, there are three options
## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
##    the most common choice, available for all microcontrollers
## 2) Code in FLASH stack in internal RAM heap in external RAM (file
##    *_xram.ld) useful for hardware like STM3220G-EVAL when big heap is
##    needed. The microcontroller must have an external memory interface.
## 3) Code + stack + heap in external RAM, (file *_all_in_xram.ld)
##    useful for debugging code in hardware like STM3220G-EVAL. Code runs
##    *very* slow compared to FLASH. Works only with a booloader that
##    forwards interrrupts @ 0x64000000 (see miosix/_tools/bootloaders for
##    one).
##    The microcontroller must have an external memory interface.
## 4) Same as 3) but space has been reserved for a process pool, allowing
##    to configure the kernel with "#define WITH_PROCESSES"
## 5) Code in FLASH, kernel stack+heap in internal RAM, ELF pool in Flash
##    and process pool in external RAM.
## 6) Code in FLASH, kernel heap, data and bss in external RAM. Kernel heap
##    (used for boot and IRQs) in internal RAM.
## Warning! enable external ram if you use a linker script that requires it
## (see the XRAM flag below)
LINKER_SCRIPT_PATH := arch/board/stm32f207ig_stm3220g-eval/
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_rom.ld
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_xram.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_all_in_xram.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_xram_processes.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_xram-data-bss-heap.ld

## Enable/disable initialization of external RAM at boot. Three options:
## __ENABLE_XRAM : If you want the heap in xram (with an appropriate linker
## script selected above)
## __ENABLE_XRAM and __CODE_IN_XRAM : Debug mode with code + stack + heap
## in xram (with an appropriate linker script selected above)
## none selected : don't use xram (with an appropriate linker script
## selected above)
XRAM := -D__ENABLE_XRAM
#XRAM := -D__ENABLE_XRAM -D__CODE_IN_XRAM
