##
## Options for board stm32f765ii_marco_ram_board
## Included by the board's makefile
##

## Linker script type
## 1)   - .text                      internal Flash
##      - IRQ stack                  internal SRAM
##      - .data, .bss                internal SRAM
##      - Heap (and thread stacks)   internal SRAM
##    The most common choice, available for all microcontrollers
#LINKER_SCRIPT := ldscripts/miosix-flash-ram.ld
#BOARD_LINKER_SCRIPT := unikernel.ld

## 2)   - .text                      internal Flash
##      - IRQ stack                  internal SRAM
##      - .data, .bss                internal SRAM
##      - Heap (and thread stacks)   external SDRAM (bank 1)
##    For application with small static buffers. Only uses SDRAM bank 1.
LINKER_SCRIPT := ldscripts/miosix-flash-2ram-separate-heap.ld
BOARD_LINKER_SCRIPT := unikernel-xram-heap.ld

## 3)   - .text                      internal Flash
##      - IRQ stack                  internal SRAM
##      - .data, .bss                external SDRAM (bank 1)
##      - Heap (and thread stacks)   external SDRAM (bank 1)
##    For application with large static buffers. Only uses SDRAM bank 1.
#LINKER_SCRIPT := ldscripts/miosix-flash-2ram-separate-irqstack.ld
#BOARD_LINKER_SCRIPT := unikernel-xram.ld

## 4)   - .text                      internal Flash
##      - IRQ stack                  internal SRAM
##      - .data, .bss                internal SRAM
##      - Heap (and thread stacks)   internal SRAM
##      - Process Pool               internal SRAM
#LINKER_SCRIPT := ldscripts/miosix-flash-ram.ld
#BOARD_LINKER_SCRIPT := processes.ld

## 5) stm32_2m+384k_xram_256M_processes.ld
##      - .text                      internal Flash
##      - IRQ stack                  internal SRAM
##      - .data, .bss                internal SRAM
##      - Heap (and thread stacks)   internal SRAM
##      - Process Pool               external SDRAM (bank 1)
#LINKER_SCRIPT := ldscripts/miosix-flash-2ram-separate-pool.ld
#BOARD_LINKER_SCRIPT := processes-xram.ld
