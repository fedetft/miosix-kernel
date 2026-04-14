##
## Options for board stm32h755zi_nucleo
## Included by the board's makefile
##

## Linker script type, there are two options
## 1) Code in FLASH, stack + heap in internal RAM (file unikernel.ld)
## 2) Same as 1) but space has been reserved for a process pool, allowing
##    to configure the kernel with "#define WITH_PROCESSES"
LINKER_SCRIPT := unikernel.ld
#LINKER_SCRIPT := processes.ld
