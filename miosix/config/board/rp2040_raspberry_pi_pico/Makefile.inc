##
## Options for board rp2040_raspberry_pi_pico
## Included by the board's makefile
##

# Board variant for selecting either the "plain" Pico or Pico W
#BOARD_VARIANT := BOARD_VARIANT_PICO
BOARD_VARIANT := BOARD_VARIANT_PICO_W

## Linker script type, there are two options
## 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
## 2) Same as 1) but space has been reserved for a process pool, allowing
##    to configure the kernel with "#define WITH_PROCESSES"
LINKER_SCRIPT_PATH := arch/board/rp2040_raspberry_pi_pico/
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)rp2040_2M+264k_rom.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)rp2040_2M+264k_processes.ld
