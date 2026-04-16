##
## CMakeLists.txt for board stm32f429zi_skyward_homeone
##

# Directory with header files for this board
set(MIOSIX_CHIP_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/chip/stm32f4)

# Linker script options
# Each script in the list may be followed by the compiler options it requires
# These options are automatically added to the C/CXX compiler command line when
# that linker script is selected
set(MIOSIX_LINKER_SCRIPT_LIST
    # 1) Code in FLASH, stack + heap in internal RAM
    unikernel.ld
    # 2) Code in FLASH, stack + heap in external RAM
    unikernel-xram.ld   -D__ENABLE_XRAM
)
set(MIOSIX_DEFAULT_LINKER_SCRIPT unikernel-xram.ld)

# Select architecture specific files
set(MIOSIX_BOARD_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/stm32f2_f4_i2c.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/stm32_hardware_rng.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/stm32_wd.cpp
    ${MIOSIX_BOARD_INC}/boot.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/bsp.cpp
)

# Add a #define to allow querying board name
set(MIOSIX_BOARD_C_FLAGS -D_BOARD_STM32F429ZI_SKYWARD_HOMEONE)
set(MIOSIX_BOARD_CXX_FLAGS -D_BOARD_STM32F429ZI_SKYWARD_HOMEONE)

# Specify a custom flash command
# This is the program that is invoked when the program-<target_name> target is
# built. Use <binary> or <hex> as placeolders, they will be replaced by the
# build systems with the binary or hex file path repectively.
# If a command is not specified, the build system will fallback to st-flash
#set(PROGRAM_CMDLINE st-flash --connect-under-reset --reset write <binary> 0x08000000)
