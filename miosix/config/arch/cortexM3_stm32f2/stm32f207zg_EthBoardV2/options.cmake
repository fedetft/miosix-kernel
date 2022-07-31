# Linker script type, there are two options:
# 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
#    the most common choice, available for all microcontrollers
# 2) Code in external RAM, stack + heap in internal RAM
#    (file *_code_in_xram.ld) useful for debugging. Code runs
#    *very* slow compared to FLASH. Works only with a booloader that
#    forwards interrrupts @ 0x60000000 (see miosix/_tools/bootloaders for
#    one).
#    You must -D__CODE_IN_XRAM below.

#set(LINKER_SCRIPT ${BOARD_PATH}/stm32_1m+128k_rom.ld)
set(LINKER_SCRIPT ${BOARD_PATH}/stm32_1m+128k_code_in_xram.ld)

# XRAM is always enabled on this board, even if the _rom linker script
# does not make explicit use of it.
# Uncommenting __CODE_IN_XRAM (with an appropriate linker script selected
# above) allows to run code from external RAM, useful for debugging

set(XRAM -D__CODE_IN_XRAM)
