# Linker script type, there are two options:
# 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
# 2) Code + stack + heap in internal RAM (file *_ram.ld)
# 3) Same as 1) but space has been reserved for a process pool, allowing ro
#    configure the kernel with "#define WITH_PROCESSES"

set(LINKER_SCRIPT ${BOARD_PATH}/stm32_1m+192k_rom.ld)
#set(LINKER_SCRIPT ${BOARD_PATH}/stm32_1m+192k_ram.ld)
#set(LINKER_SCRIPT ${BOARD_PATH}/stm32_1m+192k_rom_processes.ld)

# This causes the interrupt vector table to be relocated in SRAM, must be
# uncommented when using the ram linker script

#set(SRAM_BOOT -DVECT_TAB_SRAM)

# Select clock frequency (HSE_VALUE is the xtal on board, fixed)

set(CLOCK_FREQ -DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000)
#set(CLOCK_FREQ -DHSE_VALUE=8000000 -DSYSCLK_FREQ_100MHz=100000000)
#set(CLOCK_FREQ -DHSE_VALUE=8000000 -DSYSCLK_FREQ_84MHz=84000000)
