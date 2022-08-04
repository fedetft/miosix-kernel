# Linker script type, there are two options:
# 1) Code in FLASH, stack + heap in RAM
# 2) Code in FLASH, stack + heap in RAM flashing with bootloader

# TODO broken
#set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH}stm32_64k+20k_bootloader.ld)

## Select clock frequency
set(CLOCK_FREQ -DSYSCLK_FREQ_24MHz=24000000)
#set(CLOCK_FREQ -DSYSCLK_FREQ_36MHz=36000000)
#set(CLOCK_FREQ -DSYSCLK_FREQ_48MHz=48000000)
#set(CLOCK_FREQ -DSYSCLK_FREQ_56MHz=56000000)
#set(CLOCK_FREQ -DSYSCLK_FREQ_72MHz=72000000)
