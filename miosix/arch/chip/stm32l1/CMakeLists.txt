##
## CMakeLists.txt for chip stm32l1
##

# CPU microarchitecture
set(MIOSIX_CPU_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/armv7m)

# Set the path to the multilib directory for this architecture (used by clang)
set(MIOSIX_MULTILIB_PATH thumb/v7-m/nofp)

# Select appropriate compiler flags for both ASM/C/C++/linker
set(MIOSIX_CPU_FLAGS -mcpu=cortex-m3 -mthumb)
set(MIOSIX_CHIP_C_FLAGS -D_CHIP_STM32L1)
set(MIOSIX_CHIP_CXX_FLAGS -D_CHIP_STM32L1)

# Select architecture specific files
set(MIOSIX_CHIP_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/armv7m/interfaces-impl/cpu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/common/cortexMx_interrupts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/mpu/cortexMx_mpu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/common/cortexMx_userspace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/os_timer/stm32_16bit_os_timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/sleep/cortexMx_sleep.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/gpio/stm32_gpio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/serial/stm32f1_f2_f4_serial.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/serial/stm32_serial_common.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/delays.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/poweroff.cpp
)
