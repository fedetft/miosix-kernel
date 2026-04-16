##
## CMakeLists.txt for board stm32f746zg_nucleo
##

# Directory with header files for this board
set(MIOSIX_CHIP_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/chip/stm32f7)

# Set the path to the multilib directory for this architecture (used by clang)
set(MIOSIX_MULTILIB_PATH thumb/v7e-m+fp/hard)

# The stm32f746 lacks the double precision FPU, so we will build for m4
set(MIOSIX_CPU_FLAGS -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)

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
    ${MIOSIX_BOARD_INC}/boot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/stm32_hardware_rng.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/bsp.cpp
)

# Add a #define to allow querying board name
set(MIOSIX_BOARD_C_FLAGS -D_BOARD_STM32F746ZG_NUCLEO)
set(MIOSIX_BOARD_CXX_FLAGS -D_BOARD_STM32F746ZG_NUCLEO)

# Specify a custom flash command
# This is the program that is invoked when the program-<target_name> target is
# built. Use <binary> or <hex> as placeolders, they will be replaced by the
# build systems with the binary or hex file path repectively.
# If a command is not specified, the build system will fallback to st-flash
#set(PROGRAM_CMDLINE st-flash --connect-under-reset --reset write <binary> 0x08000000)
