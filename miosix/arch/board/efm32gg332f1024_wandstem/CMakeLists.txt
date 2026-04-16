##
## CMakeLists.txt for board efm32gg332f1024_wandstem
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
    ${MIOSIX_BOARD_INC}/interfaces-impl/spi.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/power_manager.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/os_timer.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/timer_interface.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/rtc.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/hrtb.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/gpio_timer.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/transceiver_timer.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/gpioirq.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/transceiver.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/flopsync_vht.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/vht.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/virtual_clock.cpp
)

# Add a #define to allow querying board name
set(MIOSIX_BOARD_C_FLAGS -D_BOARD_WANDSTEM)
set(MIOSIX_BOARD_CXX_FLAGS -D_BOARD_WANDSTEM)

# Specify a custom flash command
# This is the program that is invoked when the program-<target_name> target is
# built. Use <binary> or <hex> as placeolders, they will be replaced by the
# build systems with the binary or hex file path repectively.
# If a command is not specified, the build system will fallback to st-flash
set(PROGRAM_CMDLINE wandstem-flash -m u -f <binary>)
