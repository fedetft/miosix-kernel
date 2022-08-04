# Select linker script and boot file.
# Their path must be relative to the miosix directory.
set(BOOT_FILE ${BOARD_PATH}/core/stage_1_boot.cpp)

# Select architecture specific files.
# These are the files in arch/<arch name>/<board name>.
set(ARCH_SRC
        ${ARCH_ALL_COMMON_PATH}/drivers/sd_stm32f1.cpp
        ${BOARD_PATH}/interfaces-impl/bsp.cpp)

# Add a #define to allow querying board name.
list(APPEND CFLAGS_BASE -D_BOARD_MP3V2 -DSTM32F10X_HD)
list(APPEND CXXFLAGS_BASE -D_BOARD_MP3V2 -DSTM32F10X_HD)

# Clock frequency
set(CLOCK_FREQ -DSYSCLK_FREQ_72MHz=72000000)

# Select programmer command line
# This is the program that is invoked when the user types
# 'make program'
# The command must provide a way to program the board, or print an
# error message saying that 'make program' is not supported for that
# board.

if (${LINKER_SCRIPT} STREQUAL ${LINKER_SCRIPT_PATH}stm32_512k+64k_ram.ld)
    set(PROGRAM_CMDLINE "mp3v2_bootloader --ram main.bin")
else()
    set(PROGRAM_CMDLINE "mp3v2_bootloader --code main.bin")
endif()
