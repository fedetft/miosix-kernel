##
## CMakeLists.txt for chip lpc2000
##

# CPU microarchitecture
set(MIOSIX_CPU_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/armv4)

# Set the path to the multilib directory for this architecture (used by clang)
set(MIOSIX_MULTILIB_PATH arm/v4t/nofp)

# Select appropriate compiler flags for both ASM/C/C++/linker
set(MIOSIX_CPU_FLAGS -mcpu=arm7tdmi)
set(MIOSIX_CHIP_C_FLAGS -D_CHIP_LPC2000)
set(MIOSIX_CHIP_CXX_FLAGS -D_CHIP_LPC2000)

# Select architecture specific files
set(MIOSIX_CHIP_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/armv4/interfaces-impl/cpu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/armv4/interfaces-impl/interrupts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/os_timer/lpc2000_os_timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/sleep/lpc2000_sleep.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/serial/serial_lpc2000.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/sdmmc/sd_lpc2000.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/clock/lpc2000_clock.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/delays.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/poweroff.cpp
)
