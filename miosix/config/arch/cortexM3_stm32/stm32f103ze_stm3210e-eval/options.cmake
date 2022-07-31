# Linker script type, there are three options:
# 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
#    the most common choice, available for all microcontrollers
# 2) Code in FLASH stack in internal RAM heap in external RAM (file
#    *_xram.ld) useful for hardware like STM3210E-EVAL when big heap is
#    needed. The microcontroller must have an external memory interface.
# 3) Code + stack + heap in external RAM, (file *_all_in_xram.ld)
#    useful for debugging code in hardware like STM3210E-EVAL. Code runs
#    *very* slow compared to FLASH. Works only with a booloader that
#    forwards interrrupts @ 0x68000000 (see miosix/_tools/bootloaders for
#    one).
#    The microcontroller must have an external memory interface.
# Warning! enable external ram if you use a linker script that requires it
# (see the XRAM flag below)

#set(LINKER_SCRIPT ${BOARD_PATH}/stm32_512k+64k_rom.ld)
#set(LINKER_SCRIPT ${BOARD_PATH}/stm32_512k+64k_xram.ld)
set(LINKER_SCRIPT ${BOARD_PATH}/stm32_512k+64k_all_in_xram.ld)

# Enable/disable initialization of external RAM at boot. Three options:
# __ENABLE_XRAM : If you want the heap in xram (with an appropriate linker
# script selected above)
# __ENABLE_XRAM and __CODE_IN_XRAM : Debug mode with code + stack + heap
# in xram (with an appropriate linker script selected above)
# none selected : don't use xram (with an appropriate linker script
# selected above)

#set(XRAM -D__ENABLE_XRAM)
set(XRAM -D__ENABLE_XRAM -D__CODE_IN_XRAM)

# Select clock frequency
# Not defining any of these results in the internal 8MHz clock to be used

#set(CLOCK_FREQ -DSYSCLK_FREQ_24MHz=24000000)
#set(CLOCK_FREQ -DSYSCLK_FREQ_36MHz=36000000)
#set(CLOCK_FREQ -DSYSCLK_FREQ_48MHz=48000000)
#set(CLOCK_FREQ -DSYSCLK_FREQ_56MHz=56000000)
set(CLOCK_FREQ -DSYSCLK_FREQ_72MHz=72000000)
