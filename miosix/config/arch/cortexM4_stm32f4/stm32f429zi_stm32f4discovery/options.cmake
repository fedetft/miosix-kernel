# Linker script type, there are three options:
# 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
#    the most common choice, available for all microcontrollers
# 2) Code in FLASH, stack + heap in external RAM (file *8m_xram.ld)
#    You must uncomment -D__ENABLE_XRAM below in this case.
# 3) Code in FLASH, stack + heap in external RAM (file *6m_xram.ld)
#    Same as above, but leaves the upper 2MB of RAM for the LCD.

#set(LINKER_SCRIPT ${BOARD_PATH}/stm32_2m+256k_rom.ld)
#set(LINKER_SCRIPT ${BOARD_PATH}/stm32_2m+8m_xram.ld)
set(LINKER_SCRIPT ${BOARD_PATH}/stm32_2m+6m_xram.ld)

# Uncommenting __ENABLE_XRAM enables the initialization of the external
# 8MB SDRAM memory. Do not uncomment this even if you don't use a linker
# script that requires it, as it is used for the LCD framebuffer.

set(XRAM -D__ENABLE_XRAM)

# Select clock frequency. Warning: the default clock frequency for
# this board is 168MHz and not 180MHz because, due to a limitation in
# the PLL, it is not possible to generate a precise 48MHz output when
# running the core at 180MHz. If 180MHz is chosen the USB peripheral will
# NOT WORK and the SDIO and RNG will run ~6% slower (45MHz insteand of 48)

#set(CLOCK_FREQ -DHSE_VALUE=8000000 -DSYSCLK_FREQ_180MHz=180000000)
set(CLOCK_FREQ -DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000)
#set(CLOCK_FREQ -DHSE_VALUE=8000000 -DSYSCLK_FREQ_100MHz=100000000)
