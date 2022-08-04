# Select linker script and boot file
set(LINKER_SCRIPT ${BOARD_PATH}/miosix.ld)
set(BOOT_FILE ${BOARD_PATH}/core/stage_1_boot.s)

# Select architecture specific files
set(ARCH_SRC
        ${BOARD_PATH}/interfaces-impl/portability.cpp
        ${ARCH_ALL_COMMON_PATH}/drivers/sd_lpc2000.cpp
        ${BOARD_PATH}/interfaces-impl/delays.cpp
        ${BOARD_PATH}/interfaces-impl/bsp.cpp)

# Add a #define to allow querying board name
list(APPEND CFLAGS_BASE -D_BOARD_MIOSIX_BOARD)
list(APPEND CXXFLAGS_BASE -D_BOARD_MIOSIX_BOARD)
