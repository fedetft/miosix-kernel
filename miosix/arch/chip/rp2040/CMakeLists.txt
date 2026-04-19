##
## CMakeLists.txt for chip rp2040
##

# CPU microarchitecture
set(MIOSIX_CPU_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/armv6m)

# Set the path to the multilib directory for this architecture (used by clang)
set(MIOSIX_MULTILIB_PATH thumb/v6-m/nofp)

# Select appropriate compiler flags for both ASM/C/C++/linker
set(MIOSIX_CPU_FLAGS -mcpu=cortex-m0plus -mthumb)
set(MIOSIX_CHIP_C_FLAGS -D_CHIP_RP2040)
set(MIOSIX_CHIP_CXX_FLAGS -D_CHIP_RP2040)

# Select architecture specific files
set(MIOSIX_CHIP_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/armv6m/interfaces-impl/cpu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/common/cortexMx_interrupts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/common/cortexMx_mpu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/common/cortexMx_userspace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/os_timer/rp2040_os_timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/sleep/cortexMx_sleep.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/serial/arm_pl011_serial.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/serial/rp2040_serial.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/spi/arm_pl022_spi.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/spi/rp2040_spi.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/rp2040_dma.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/delays.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/smp.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/atomic_ops_smp_impl.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/poweroff.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/CMSIS/Device/RaspberryPi/RP2040/Source/system_RP2040.c
)
