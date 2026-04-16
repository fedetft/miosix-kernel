##
## CMakeLists.txt for chip stm32f7
##

# CPU microarchitecture
set(MIOSIX_CPU_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/armv7m)

# Select appropriate compiler flags for both ASM/C/C++/linker
set(MIOSIX_CHIP_C_FLAGS -D_CHIP_STM32F7)
set(MIOSIX_CHIP_CXX_FLAGS -D_CHIP_STM32F7)

# Select architecture specific files
set(MIOSIX_CHIP_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/armv7m/interfaces-impl/cpu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/common/cortexMx_interrupts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/mpu/cortexMx_mpu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/common/cortexMx_userspace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/os_timer/stm32_32bit_os_timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/sleep/cortexMx_sleep.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/cache/cortexMx_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/gpio/stm32_gpio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/serial/stm32f7_serial.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/serial/stm32_serial_common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/dcc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/sdmmc/sd_stm32f2_f4_f7.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/delays.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/poweroff.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/CMSIS/Device/ST/STM32F7xx/Source/Templates/system_stm32f7xx.cpp
)
