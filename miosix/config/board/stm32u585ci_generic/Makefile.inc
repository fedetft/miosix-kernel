##
## Options for board stm32u585ci_generic
## Included by the board's makefile
##

## Select linker script
LINKER_SCRIPT_PATH := arch/cortexM33_stm32u5/stm32u585ci_generic
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)/stm32_2048k+768k_rom.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)/stm32_2048k+768k_processes.ld
