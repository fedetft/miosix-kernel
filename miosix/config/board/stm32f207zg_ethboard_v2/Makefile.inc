##
## Options for board stm32f207zg_ethboard_v2
## Included by the board's makefile
##

## Linker script type
## 1) Code in FLASH, all kernel data in internal RAM, external RAM reserved
##    for the process pool (file *_xram_processes.ld) to configure the
##    kernel with "#define WITH_PROCESSES"
## 2) Code in FLASH .data .bss and interrupt stack in internal RAM, heap in
##    external RAM (file *_xram.ld). Kernel runs slower but has more RAM
## 3) Code + stack + heap in external RAM, (file *_all_in_xram.ld)
##    useful for debugging code. Code runs *very* slow compared to FLASH.
##    Works only with a booloader that forwards interrrupts @ 0x6000000
##    (see miosix/_tools/bootloaders for one). The microcontroller must have
##    an external memory interface. You must -D__CODE_IN_XRAM below.
LINKER_SCRIPT_PATH := arch/board/stm32f207zg_ethboard_v2/
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_xram_processes.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_xram.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+128k_all_in_xram.ld

## XRAM is always enabled on this board, if you don't want the kernel to
## use it select the xram_processes linker script
## Uncommenting __CODE_IN_XRAM (with an appropriate linker script selected
## above) allows to run code from external RAM, useful for debugging
#XRAM := -D__CODE_IN_XRAM
