# Select linker script and boot file.
# Their path must be relative to the miosix directory.
set(LINKER_SCRIPT ${BOARD_PATH}/stm32_1M+128k_rom.ld)
set(BOOT_FILE ${BOARD_PATH}/core/stage_1_boot.cpp)

# Select architecture specific files.
# These are the files in arch/<arch name>/<board name>.
set(ARCH_SRC
        ${ARCH_ALL_COMMON_PATH}/drivers/stm32f2_f4_i2c.cpp
        ${BOARD_PATH}/interfaces-impl/delays.cpp
        ${BOARD_PATH}/interfaces-impl/bsp.cpp)

# Add a #define to allow querying board name.
list(APPEND CFLAGS_BASE -D_BOARD_SONY_NEWMAN)
list(APPEND CXXFLAGS_BASE -D_BOARD_SONY_NEWMAN)

# Clock frequency.
set(CLOCK_FREQ -DHSE_VALUE=26000000)

# Select programmer command line.
# This is the program that is invoked when the user types
# 'make program'.
# The command must provide a way to program the board, or print an
# error message saying that 'make program' is not supported for that
# board.
# The magic.bin is somewhat used by the bootloader to detect a good fw
set(PROGRAM_CMDLINE "perl -e 'print \"\\xe7\\x91\\x11\\xc0\"' > magic.bin; \
                            dfu-util -d 0fce:f0fa -a 0 -i 0 -s 0x08040000 -D main.bin;     \
                            dfu-util -d 0fce:f0fa -a 0 -i 0 -s 0x080ffffc -D magic.bin;    \
                            rm magic.bin")
