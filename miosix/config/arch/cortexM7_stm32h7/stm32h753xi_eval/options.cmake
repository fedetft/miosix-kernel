# Linker script type, there are three options:
# 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
#    the most common choice, available for all microcontrollers
# 2) Code in FLASH, stack + heap in external RAM (file *m_xram.ld)
#    You must uncomment -D__ENABLE_XRAM below in this case.

set(LINKER_SCRIPT ${BOARD_PATH}/stm32_2m+512k_rom.ld)
#set(LINKER_SCRIPT ${BOARD_PATH}/stm32_2m+32m_xram.ld)
