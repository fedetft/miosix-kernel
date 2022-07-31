# Select linker script and boot file.
# Their path must be relative to the miosix directory.
set(LINKER_SCRIPT ${BOARD_PATH}/stm32_1024k+256k_rom.ld)
set(BOOT_FILE ${BOARD_PATH}/core/stage_1_boot.cpp)

# Select architecture specific files.
# These are the files in arch/<arch name>/<board name>.
set(ARCH_SRC
        ${ARCH_ALL_COMMON_PATH}/drivers/stm32_hardware_rng.cpp
        ${BOARD_PATH}/interfaces-impl/bsp.cpp)

# Add a #define to allow querying board name.
list(APPEND CFLAGS_BASE -D_BOARD_STM32F746ZG_NUCLEO)
list(APPEND CXXFLAGS_BASE -D_BOARD_STM32F746ZG_NUCLEO)

# Select clock frequency (HSE_VALUE is the xtal on board, fixed).
set(CLOCK_FREQ -DHSE_VALUE=8000000 -DSYSCLK_FREQ_216MHz=216000000)

## Select appropriate compiler flags for both ASM/C/C++/linker
## Not all stm32f7 have the double precision FPU. Those that only
## support single precision are built as cortex m4
set(ARCH_OPTS -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)

# Select programmer command line.
# This is the program that is invoked when the user types
# 'make program'.
# The command must provide a way to program the board, or print an
# error message saying that 'make program' is not supported for that
# board.

set(PROGRAM_CMDLINE "echo 'make program not supported.'")
