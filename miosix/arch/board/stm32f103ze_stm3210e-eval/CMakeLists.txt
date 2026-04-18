##
## CMakeLists.txt for board stm32f103ze_stm3210e-eval
##

# Directory with header files for this board
set(MIOSIX_CHIP_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/chip/stm32f1)

# Linker script options
# Each script in the list may be followed by the compiler options it requires
# These options are automatically added to the C/CXX compiler command line when
# that linker script is selected
set(MIOSIX_LINKER_SCRIPT_LIST
    # 1) Code in FLASH, stack + heap in internal RAM
    unikernel.ld
    # 2) Code in FLASH, IRQ stack, .data, .bss in internal RAM, heap in external RAM
    unikernel-xram-heap.ld      -D__ENABLE_XRAM
    # 3) Code + stack + heap in external RAM, Code runs *very* slow. Works only
    #    with a booloader that forwards interrrupts @ 0x68000000
    #    (see miosix/tools/bootloaders for one).
    unikernel-all-in-xram.ld    -D__ENABLE_XRAM -D__CODE_IN_XRAM
    # 4) Code in FLASH, Kernel in internal RAM, Process pool in XRAM
    processes-xram.ld           -DWITH_PROCESSES -D__ENABLE_XRAM
)
set(MIOSIX_DEFAULT_LINKER_SCRIPT unikernel-xram-heap.ld)

# Select architecture specific files
set(MIOSIX_BOARD_SRC
    ${MIOSIX_BOARD_INC}/boot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/sdmmc/stm32f1_sd.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/bsp.cpp
)

# Add a #define to allow querying board name
set(MIOSIX_BOARD_C_FLAGS -D_BOARD_STM3210E_EVAL)
set(MIOSIX_BOARD_CXX_FLAGS -D_BOARD_STM3210E_EVAL)

# Specify a custom flash command
# This is the program that is invoked when the program-<target_name> target is
# built. Use <binary> or <hex> as placeolders, they will be replaced by the
# build systems with the binary or hex file path repectively.
# If a command is not specified, the build system will fallback to st-flash
#set(PROGRAM_CMDLINE ...)
