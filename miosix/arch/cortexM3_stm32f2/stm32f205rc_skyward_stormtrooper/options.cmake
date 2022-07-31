# Select linker script and boot file.
# Their path must be relative to the miosix directory.
set(LINKER_SCRIPT ${BOARD_PATH}/stm32_512k+128k_rom.ld)
set(BOOT_FILE ${BOARD_PATH}/core/stage_1_boot.cpp)

# Select architecture specific files.
# These are the files in arch/<arch name>/<board name>.
set(ARCH_SRC
        ${ARCH_ALL_COMMON_PATH}/drivers/stm32f2_f4_i2c.cpp
        ${BOARD_PATH}/interfaces-impl/delays.cpp
        ${BOARD_PATH}/interfaces-impl/bsp.cpp)

# Add a #define to allow querying board name.
list(APPEND CFLAGS_BASE -D_BOARD_STM32F205RC_SKYWARD_STORMTROOPER)
list(APPEND CXXFLAGS_BASE -D_BOARD_STM32F205RC_SKYWARD_STORMTROOPER)

# Clock frequency.
set(CLOCK_FREQ -DHSE_VALUE=25000000 -DSYSCLK_FREQ_120MHz=120000000)
