##
## Options for board stm32f765ii_marco_ram_board
## Included by the board's makefile
##

## Linker script type
## 1) stm32_2M+384k_ram.ld
##      - .text                      internal Flash
##      - IRQ stack                  internal SRAM
##      - .data, .bss                internal SRAM
##      - Heap (and thread stacks)   internal SRAM
##    The most common choice, available for all microcontrollers
## 2) stm32_2M+384k_xram-heap_256M.ld
##      - .text                      internal Flash
##      - IRQ stack                  internal SRAM
##      - .data, .bss                internal SRAM
##      - Heap (and thread stacks)   external SDRAM (bank 1)
##    For application with small static buffers. Only uses SDRAM bank 1.
## 3) stm32_2M+384k_xram_256M.ld
##      - .text                      internal Flash
##      - IRQ stack                  internal SRAM
##      - .data, .bss                external SDRAM (bank 1)
##      - Heap (and thread stacks)   external SDRAM (bank 1)
##    For application with large static buffers. Only uses SDRAM bank 1.
## 4) stm32_2m+384k_ram_processes.ld
##      - .text                      internal Flash
##      - IRQ stack                  internal SRAM (low 128K)
##      - .data, .bss                internal SRAM (low 128K)
##      - Heap (and thread stacks)   internal SRAM (low 128K)
##      - Process Pool               internal SRAM (high 256K)
## 5) stm32_2m+384k_xram_256M_processes.ld
##      - .text                      internal Flash
##      - IRQ stack                  internal SRAM
##      - .data, .bss                internal SRAM
##      - Heap (and thread stacks)   internal SRAM
##      - Process Pool               external SDRAM (bank 1)
LINKER_SCRIPT_PATH := arch/cortexM7_stm32f7/stm32f765ii_marco_ram_board
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)/stm32_2M+384k_ram.ld
LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)/stm32_2M+384k_xram-heap_256M.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)/stm32_2M+384k_xram_256M.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)/stm32_2m+384k_ram_processes.ld
#LINKER_SCRIPT := $(LINKER_SCRIPT_PATH)/stm32_2m+384k_xram_256M_processes.ld
