##
## CMakeLists.txt for board efm32gg990f1024_dk3750
##

# Directory with header files for this board
set(MIOSIX_CHIP_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/chip/efm32gg)

# Linker script options
# Each script in the list may be followed by the compiler options it requires
# These options are automatically added to the C/CXX compiler command line when
# that linker script is selected
set(MIOSIX_LINKER_SCRIPT_LIST
    unikernel.ld
    processes.ld    -DWITH_PROCESSES
)
set(MIOSIX_DEFAULT_LINKER_SCRIPT unikernel.ld)

# Select architecture specific files
set(MIOSIX_BOARD_SRC
    ${MIOSIX_BOARD_INC}/interfaces-impl/bsp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/os_timer/efm32_os_timer.cpp
)

# Add a #define to allow querying board name
set(MIOSIX_BOARD_C_FLAGS -D_BOARD_EFM32GG990F1024_DK3750)
set(MIOSIX_BOARD_CXX_FLAGS -D_BOARD_EFM32GG990F1024_DK3750)

# Specify a custom flash command
# This is the program that is invoked when the program-<target_name> target is
# built. Use <binary> or <hex> as placeolders, they will be replaced by the
# build systems with the binary or hex file path repectively.
# If a command is not specified, the build system will fallback to st-flash
set(PROGRAM_CMDLINE openocd -f "${MIOSIX_BOARD_INC}/openocd.cfg" -c "program \"<binary>\" 0x0 verify reset" -c shutdown)
