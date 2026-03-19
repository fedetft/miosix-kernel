##
## Options for board stm32f407vg_stm32f4discovery
## Included by the board's makefile
##

## Linker script type, there are two options
## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
## 2) Code + stack + heap in internal RAM (file *_ram.ld)
## 3) Same as 1) but space has been reserved for a process pool, allowing
##    to configure the kernel with "#define WITH_PROCESSES"
LINKER_SCRIPT_PATH := arch/cortexM4_stm32f4/stm32f407vg_stm32f4discovery/
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+192k_rom.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_1m+192k_rom_processes.ld
