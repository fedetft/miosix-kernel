# Linker script type, there are two options:
# 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
# 2) Code + stack + heap in internal RAM (file *_ram.ld)
# Note: this board relies on a bootloader for interrupt forwarding in ram

set(LINKER_SCRIPT ${BOARD_PATH}/stm32_512k+64k_rom.ld)
#set(LINKER_SCRIPT ${BOARD_PATH}/stm32_512k+64k_ram.ld)
