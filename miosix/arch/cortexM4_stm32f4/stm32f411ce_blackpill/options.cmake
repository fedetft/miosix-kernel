# Select linker script and boot file.
# Their path must be relative to the miosix directory.
set(LINKER_SCRIPT ${BOARD_PATH}/stm32_512k+128k_rom.ld)
set(BOOT_FILE ${BOARD_PATH}/core/stage_1_boot.cpp)

# Select architecture specific files.
# These are the files in arch/<arch name>/<board name>.
set(ARCH_SRC ${BOARD_PATH}/interfaces-impl/bsp.cpp)

# Add a #define to allow querying board name.
list(APPEND CFLAGS_BASE -D_BOARD_STM32F411CE_BLACKPILL)
list(APPEND CXXFLAGS_BASE -D_BOARD_STM32F411CE_BLACKPILL)

# Select clock frequency (HSE_VALUE is the xtal on board, fixed).
set(CLOCK_FREQ -DHSE_VALUE=25000000 -DSYSCLK_FREQ_100MHz=100000000)

# Select programmer command line.
# This is the program that is invoked when the user types
# 'make program'.
# The command must provide a way to program the board, or print an
# error message saying that 'make program' is not supported for that
# board.

set(PROGRAM_CMDLINE "qstlink2 -cqewV ./main.bin")
