##
## Options for board stm32f746zg_nucleo
## Included by the board's makefile
##

## Linker script type, there are two options
## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
## 2) Same as 1) but space has been reserved for a process pool, allowing
##    to configure the kernel with "#define WITH_PROCESSES"
LINKER_SCRIPT_PATH := arch/cortexM7_stm32f7/stm32f746zg_nucleo/
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+256k_rom.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+320k_rom_processes.ld
