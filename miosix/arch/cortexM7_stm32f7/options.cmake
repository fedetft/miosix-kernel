# Select architecture specific files
list(APPEND ARCH_SRC
        ${ARCH_ALL_COMMON_PATH}/core/interrupts_cortexMx.cpp
        ${ARCH_ALL_COMMON_PATH}/core/mpu_cortexMx.cpp
        ${ARCH_ALL_COMMON_PATH}/core/cache_cortexMx.cpp
        ${ARCH_ALL_COMMON_PATH}/drivers/serial_stm32.cpp
        ${ARCH_ALL_COMMON_PATH}/drivers/sd_stm32f2_f4.cpp
        ${ARCH_ALL_COMMON_PATH}/drivers/dcc.cpp
        ${ARCH_PATH}/common/interfaces-impl/portability.cpp
        ${ARCH_PATH}/common/interfaces-impl/delays.cpp
        ${ARCH_PATH}/common/interfaces-impl/gpio_impl.cpp
        ${ARCH_ALL_COMMON_PATH}/CMSIS/Device/ST/STM32F7xx/Source/Templates/system_stm32f7xx.c)

# Select appropriate compiler flags for both ASM / C / C++ / linker

# TODO: when upgrading compiler, add native cortex M7 support.
# For now we'll fallback to M4 which is compatible, but does not support
# double precision HW floating point.

list(APPEND AFLAGS_BASE ${ARCHOPTS})
list(APPEND CFLAGS_BASE -D_ARCH_CORTEXM7_STM32F7 ${CLOCK_FREQ} ${XRAM} ${SRAM_BOOT} ${ARCHOPTS} ${OPT_OPTIMIZATION} -c)
list(APPEND CXXFLAGS_BASE -D_ARCH_CORTEXM7_STM32F7 ${CLOCK_FREQ} ${XRAM} ${SRAM_BOOT} ${ARCHOPTS} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -c)
list(APPEND LFLAGS_BASE ${ARCHOPTS} -Wl,--gc-sections,-Map=main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib)
