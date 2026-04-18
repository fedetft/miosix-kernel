##
## CMakeLists.txt for chip atsam4l
##

# CPU microarchitecture
set(MIOSIX_CPU_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/armv7m)

# Set the path to the multilib directory for this architecture (used by clang)
set(MIOSIX_MULTILIB_PATH thumb/v7-m/nofp)

# Select appropriate compiler flags for both ASM/C/C++/linker
set(MIOSIX_CPU_FLAGS -mcpu=cortex-m4 -mthumb)
set(MIOSIX_CHIP_C_FLAGS -D_CHIP_ATSAM4L)
set(MIOSIX_CHIP_CXX_FLAGS -D_CHIP_ATSAM4L)

# Select architecture specific files
set(MIOSIX_CHIP_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/armv7m/interfaces-impl/cpu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/common/cortexMx_interrupts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/mpu/cortexMx_mpu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/cpu/common/cortexMx_userspace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/os_timer/atsam4l_os_timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/sleep/cortexMx_sleep.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/serial/atsam4l_serial.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/clock/atsam4l_clock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/atsam4l_lcd.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/sleep.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/arch/drivers/gpio/atsam4l_gpio.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/delays.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/poweroff.cpp
)
