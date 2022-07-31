# Select linker script and boot file.
# Their path must be relative to the miosix directory.
set(LINKER_SCRIPT ${BOARD_PATH}/stm32_256k+64k_rom.ld)
set(BOOT_FILE ${BOARD_PATH}/core/stage_1_boot.cpp)

# Select architecture specific files.
# These are the files in arch/<arch name>/<board name>.
set(ARCH_SRC
        ${ARCH_ALL_COMMON_PATH}/drivers/stm32f2_f4_i2c.cpp
        ${BOARD_PATH}/interfaces-impl/bsp.cpp)

# Add a #define to allow querying board name.
list(APPEND CFLAGS_BASE -D_BOARD_STM32F401VC_STM32F4DISCOVERY)
list(APPEND CXXFLAGS_BASE -D_BOARD_STM32F401VC_STM32F4DISCOVERY)

# Select clock frequency (HSE_VALUE is the xtal on board, fixed).
set(CLOCK_FREQ -DHSE_VALUE=8000000 -DSYSCLK_FREQ_84MHz=84000000)

# Select programmer command line.
# This is the program that is invoked when the user types
# 'make program'.
# The command must provide a way to program the board, or print an
# error message saying that 'make program' is not supported for that
# board.

set(PROGRAM_CMDLINE "qstlink2 -cqewV ./main.bin")
