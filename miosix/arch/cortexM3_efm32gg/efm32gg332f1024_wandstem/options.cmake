# Select linker script and boot file.
# Their path must be relative to the miosix directory.
set(LINKER_SCRIPT ${BOARD_PATH}/efm32_1M+128k_rom_usbbootloader.ld)
set(BOOT_FILE ${BOARD_PATH}/core/stage_1_boot.cpp)

# Select architecture specific files.
# These are the files in arch/<arch name>/<board name>.
set(ARCH_SRC
        ${BOARD_PATH}/interfaces-impl/bsp.cpp
        ${BOARD_PATH}/interfaces-impl/spi.cpp
        ${BOARD_PATH}/interfaces-impl/power_manager.cpp
        ${BOARD_PATH}/interfaces-impl/gpioirq.cpp
        ${BOARD_PATH}/interfaces-impl/hardware_timer.cpp
        ${BOARD_PATH}/interfaces-impl/transceiver.cpp)

# Add a #define to allow querying board name.
list(APPEND CFLAGS_BASE -DEFM32GG332F1024 -D_BOARD_WANDSTEM)
list(APPEND CXXFLAGS_BASE -DEFM32GG332F1024 -D_BOARD_WANDSTEM)

# Clock frequency
set(CLOCK_FREQ -DEFM32_HFXO_FREQ=48000000 -DEFM32_LFXO_FREQ=32768)

# Select programmer command line.
# This is the program that is invoked when the user types
# 'make program'.
# The command must provide a way to program the board, or print an
# error message saying that 'make program' is not supported for that
# board.

set(PROGRAM_CMDLINE "echo 'make program not supported.'")
