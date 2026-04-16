##
## CMakeLists.txt for board stm32f429zi_oledboard2
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
    unikernel-xram-8m.ld
    # 3) Same as 2), but leaves the upper 2MB of RAM for the LCD framebuffers
    unikernel-xram-6m.ld
)
set(MIOSIX_DEFAULT_LINKER_SCRIPT unikernel-xram-6m.ld)

# Select architecture specific files
set(MIOSIX_BOARD_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/stm32_hardware_rng.cpp
    ${MIOSIX_BOARD_INC}/boot.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/bsp.cpp
)

# Add a #define to allow querying board name
# Do not uncomment this even if you don't use a linker script that requires
# it, as it is used for the LCD framebuffer.
set(MIOSIX_BOARD_C_FLAGS -D_BOARD_STM32F429ZI_OLEDBOARD2 -D__ENABLE_XRAM)
set(MIOSIX_BOARD_CXX_FLAGS -D_BOARD_STM32F429ZI_OLEDBOARD2 -D__ENABLE_XRAM)

# Specify a custom flash command
# This is the program that is invoked when the program-<target_name> target is
# built. Use <binary> or <hex> as placeolders, they will be replaced by the
# build systems with the binary or hex file path repectively.
# If a command is not specified, the build system will fallback to st-flash
set(PROGRAM_CMDLINE stm32flash -w <binary> -v /dev/ttyUSB0)
