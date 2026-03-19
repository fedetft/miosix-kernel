##
## Options for board efm32gg332f1024_wandstem
## Included by the board's makefile
##

## Linker script type, there are two options
## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
## 2) Same as 1) but space has been reserved for a process pool, allowing
##    to configure the kernel with "#define WITH_PROCESSES"
LINKER_SCRIPT_PATH := arch/cortexM3_efm32gg/efm32gg332f1024_wandstem/
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)efm32_1M+128k_rom_usbbootloader.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)efm32_1M+128k_rom_usbbootloader_processes.ld
