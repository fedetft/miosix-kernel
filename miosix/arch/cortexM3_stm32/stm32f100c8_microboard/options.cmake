# Select linker script and boot file.
# Their path must be relative to the miosix directory.
set(LINKER_SCRIPT ${BOARD_PATH}/stm32_63k+8k_rom.ld)
set(BOOT_FILE ${BOARD_PATH}/core/stage_1_boot.cpp)

# Select architecture specific files.
# These are the files in arch/<arch name>/<board name>
set(ARCH_SRC
        ${BOARD_PATH}/interfaces-impl/bsp.cpp
        ${BOARD_PATH}/drivers/rtc.cpp)

# Add a #define to allow querying board name.
list(APPEND CFLAGS_BASE -D_BOARD_MICROBOARD -DSTM32F10X_MD_VL)
list(APPEND CXXFLAGS_BASE -D_BOARD_MICROBOARD -DSTM32F10X_MD_VL)

# Clock frequency.
# Not defining anything results in HSI being used.
set(CLOCK_FREQ -DSYSCLK_FREQ_24MHz=24000000 -DRUN_WITH_HSI)

# Select programmer command line.
# This is the program that is invoked when the user types
# 'make program'.
# The command must provide a way to program the board, or print an
# error message saying that 'make program' is not supported for that
# board.
set(PROGRAM_CMDLINE "stm32flash -w main.hex -v /dev/ttyUSB0")
