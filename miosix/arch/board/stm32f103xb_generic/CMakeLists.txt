##
## CMakeLists.txt for board stm32f103xb_generic
##

# Directory with header files for this board
set(MIOSIX_CHIP_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/chip/stm32f1)

# Linker script options
# Each script in the list may be followed by the compiler options it requires
# These options are automatically added to the C/CXX compiler command line when
# that linker script is selected
set(MIOSIX_LINKER_SCRIPT_LIST
    # stm32f103c8 has 64K, stm32f103cb has 128K
    unikernel-64k+20k.ld
    unikernel-128k+20k.ld
)
set(MIOSIX_DEFAULT_LINKER_SCRIPT unikernel-128k+20k.ld)

# Select architecture specific files
set(MIOSIX_BOARD_SRC
    ${MIOSIX_BOARD_INC}/boot.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/bsp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/stm32_rtc.cpp
)

# Add a #define to allow querying board name
set(MIOSIX_BOARD_C_FLAGS -D_BOARD_STM32F103XB_GENERIC)
set(MIOSIX_BOARD_CXX_FLAGS -D_BOARD_STM32F103XB_GENERIC)

# Specify a custom flash command
# This is the program that is invoked when the program-<target_name> target is
# built. Use <binary> or <hex> as placeolders, they will be replaced by the
# build systems with the binary or hex file path repectively.
# If a command is not specified, the build system will fallback to st-flash
set(PROGRAM_CMDLINE stm32flash -w <binary> -v /dev/ttyUSB0)
