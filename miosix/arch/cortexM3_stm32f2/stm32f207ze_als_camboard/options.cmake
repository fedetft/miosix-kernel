# Select linker script and boot file.
# Their path must be relative to the miosix directory.
set(LINKER_SCRIPT ${BOARD_PATH}/stm32_1m+128k_rom.ld)
set(BOOT_FILE ${BOARD_PATH}/core/stage_1_boot.cpp)

# Select architecture specific files.
# These are the files in arch/<arch name>/<board name>.
set(ARCH_SRC
        ${BOARD_PATH}/interfaces-impl/delays.cpp
        ${BOARD_PATH}/interfaces-impl/bsp.cp)

# Add a #define to allow querying board name.
set(CFLAGS_BASE "${CFLAGS_BASE} -D_BOARD_ALS_CAMBOARD")
set(CXXFLAGS_BASE "${CXXFLAGS_BASE} -D_BOARD_ALS_CAMBOARD")

# Clock frequency.
set(CLOCK_FREQ -DHSE_VALUE=8000000 -DSYSCLK_FREQ_120MHz=120000000)

# XRAM is always enabled in this board.
list(APPEND XRAM -D__ENABLE_XRAM)

# Select programmer command line.
# This is the program that is invoked when the user types
# 'make program'.
# The command must provide a way to program the board, or print an
# error message saying that 'make program' is not supported for that
# board.

set(PROGRAM_CMDLINE "stm32flash -w main.hex -v /dev/ttyUSB1")
