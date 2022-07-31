# Select architecture specific files
list(APPEND ARCH_SRC
        ${ARCH_ALL_COMMON_PATH}/core/interrupts_cortexMx.cpp
        ${ARCH_ALL_COMMON_PATH}/drivers/serial_stm32.cpp
        ${ARCH_PATH}/common/interfaces-impl/portability.cpp
        ${ARCH_PATH}/common/interfaces-impl/gpio_impl.cpp
        ${ARCH_PATH}/common/interfaces-impl/delays.cpp
        ${ARCH_ALL_COMMON_PATH}/CMSIS/Device/ST/STM32L1xx/Source/Templates/system_stm32l1xx.c)

# Select appropriate compiler flags for both ASM / C / C++ / linker
list(APPEND AFLAGS_BASE -mcpu=cortex-m3 -mthumb)
list(APPEND CFLAGS_BASE -D_ARCH_CORTEXM3_STM32L1 ${CLOCK_FREQ} ${XRAM} -mcpu=cortex-m3 -mthumb ${OPT_OPTIMIZATION} -c)
list(APPEND CXXFLAGS_BASE -D_ARCH_CORTEXM3_STM32L1 ${CLOCK_FREQ} ${XRAM} ${OPT_EXCEPT} -mcpu=cortex-m3 -mthumb ${OPT_OPTIMIZATION} -c)
list(APPEND LFLAGS_BASE -mcpu=cortex-m3 -mthumb -Wl,--gc-sections,-Map=main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib)
