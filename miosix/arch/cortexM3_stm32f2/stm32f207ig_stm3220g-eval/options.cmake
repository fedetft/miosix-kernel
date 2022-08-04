# Select linker script and boot file.
# Their path must be relative to the miosix directory.
set(BOOT_FILE ${BOARD_PATH}/core/stage_1_boot.cpp)

# Select architecture specific files.
set(ARCH_SRC
        ${ARCH_ALL_COMMON_PATH}/drivers/sd_stm32f2_f4.cpp
        ${BOARD_PATH}/interfaces-impl/delays.cpp
        ${BOARD_PATH}/interfaces-impl/bsp.cpp)

# Add a #define to allow querying board name.
list(APPEND CFLAGS_BASE -D_BOARD_STM3220G_EVAL)
list(APPEND CXXFLAGS_BASE -D_BOARD_STM3220G_EVAL)

# Clock frequency.
set(CLOCK_FREQ -DHSE_VALUE=25000000 -DSYSCLK_FREQ_120MHz=120000000)

# Select programmer command line.
# This is the program that is invoked when the user types
# 'make program'.
# The command must provide a way to program the board, or print an
# error message saying that 'make program' is not supported for that
# board.

if (${LINKER_SCRIPT} STREQUAL ${LINKER_SCRIPT_PATH}stm32_1m+128k_all_in_xram.ld)
    set(PROGRAM_CMDLINE "${KPATH}/_tools/bootloaders/stm32/pc_loader/pc_loader /dev/ttyUSB0 main.bin")
else()
    set(PROGRAM_CMDLINE "qstlink2 -cqewV ./main.bin")
endif()
