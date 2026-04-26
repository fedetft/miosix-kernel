##
## CMakeLists.txt for chip stm32u5
##

# CPU microarchitecture
set(MIOSIX_CPU_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/armv8m)

# Set the path to the multilib directory for this architecture (used by clang)
set(MIOSIX_MULTILIB_PATH thumb/v8-m.main+fp/hard)

# Select appropriate compiler flags for both ASM/C/C++/linker
set(MIOSIX_CPU_FLAGS -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)
set(MIOSIX_CHIP_C_FLAGS -D_CHIP_STM32U5)
set(MIOSIX_CHIP_CXX_FLAGS -D_CHIP_STM32U5)

# Select architecture specific files
set(MIOSIX_CHIP_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/armv8m/interfaces-impl/cpu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/common/cortexMx_interrupts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/common/cortexMx_mpu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/common/cortexMx_userspace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/os_timer/stm32_32bit_os_timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/sleep/cortexMx_sleep.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/serial/stm32u5_serial.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/serial/stm32_serial_common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/gpio/stm32_gpio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/dcc.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/delays.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/poweroff.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/clock/stm32u5_pll.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/CMSIS/Device/ST/STM32U5xx/Source/Templates/system_stm32u5xx.cpp
)
