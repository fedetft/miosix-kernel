##
## Options for board stm32l4r9zi_sensortile
## Included by the board's makefile
##

## Linker script type, there are two options
## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
## 2) Same as 1) but space has been reserved for a process pool, allowing
##    to configure the kernel with "#define WITH_PROCESSES"
LINKER_SCRIPT_PATH := arch/board/stm32l4r9zi_sensortile/
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+640k_rom.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)stm32_2m+640k_rom_processes.ld
