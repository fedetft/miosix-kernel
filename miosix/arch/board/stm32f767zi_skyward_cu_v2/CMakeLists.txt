##
## CMakeLists.txt for board stm32f767zi_skyward_cu_v2
##

# Directory with header files for this board
set(MIOSIX_CHIP_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/chip/stm32f7)

# Set the path to the multilib directory for this architecture (used by clang)
set(MIOSIX_MULTILIB_PATH thumb/v7e-m+dp/hard)

# The stm32f767 has the double precision FPU, so we will build for m7
set(MIOSIX_CPU_FLAGS -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16)

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
    ${MIOSIX_BOARD_INC}/interfaces-impl/bsp.cpp
)

# Add a #define to allow querying board name
set(MIOSIX_BOARD_C_FLAGS -D_BOARD_STM32F767ZI_SKYWARD_CU_V2)
set(MIOSIX_BOARD_CXX_FLAGS -D_BOARD_STM32F767ZI_SKYWARD_CU_V2)

# Specify a custom flash command
# This is the program that is invoked when the program-<target_name> target is
# built. Use <binary> or <hex> as placeolders, they will be replaced by the
# build systems with the binary or hex file path repectively.
# If a command is not specified, the build system will fallback to st-flash
# set(PROGRAM_CMDLINE st-flash --connect-under-reset --reset write <binary> 0x8000000)
