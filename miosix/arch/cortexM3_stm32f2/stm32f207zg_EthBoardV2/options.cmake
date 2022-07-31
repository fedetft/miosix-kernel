# Select linker script and boot file.
# Their path must be relative to the miosix directory.
set(BOOT_FILE ${BOARD_PATH}/core/stage_1_boot.cpp)

# Select architecture specific files.
set(ARCH_SRC
        ${ARCH_ALL_COMMON_PATH}/drivers/sd_stm32f2_f4.cpp
        ${BOARD_PATH}/interfaces-impl/delays.cpp
        ${BOARD_PATH}/interfaces-impl/bsp.cpp)

# Add a #define to allow querying board name.
list(APPEND CFLAGS_BASE -D_BOARD_ETHBOARDV2)
list(APPEND CXXFLAGS_BASE -D_BOARD_ETHBOARDV2)

# Clock frequency.
set(CLOCK_FREQ -DHSE_VALUE=25000000 -DSYSCLK_FREQ_120MHz=120000000)

# XRAM is always enabled in this board.
list(APPEND XRAM -D__ENABLE_XRAM)

# Select programmer command line.
# This is the program that is invoked when the user types
# 'make program'.
# The command must provide a way to program the board, or print an
# error message saying that 'make program' is not supported for that
# board.

if (${LINKER_SCRIPT} STREQUAL ${LINKER_SCRIPT_PATH}stm32_1m+128k_code_in_xram.ld)
    set(PROGRAM_CMDLINE "${KPATH}/_tools/bootloaders/stm32/pc_loader/pc_loader /dev/ttyUSB1 main.bin")
else()
    set(PROGRAM_CMDLINE "stm32flash -w main.hex -v /dev/ttyUSB1")
endif()
