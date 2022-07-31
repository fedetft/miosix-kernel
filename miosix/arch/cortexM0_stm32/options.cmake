# Select architecture specific files
list(APPEND ARCH_SRC
        ${ARCH_ALL_COMMON_PATH}/core/interrupts_cortexMx.cpp
        ${ARCH_ALL_COMMON_PATH}/arch/common/drivers/serial_stm32.cpp
        ${ARCH_PATH}/interfaces-impl/portability.cpp
        ${ARCH_PATH}/interfaces-impl/delays.cpp
        ${ARCH_PATH}/interfaces-impl/gpio_impl.cpp
        ${ARCH_ALL_COMMON_PATH}/CMSIS/Device/ST/STM32F0xx/Source/Templates/system_stm32f0xx.c)

# Select appropriate compiler flags for both ASM / C / C++ / linker
list(APPEND AFLAGS_BASE -mcpu=cortex-m0 -mthumb)
list(APPEND CFLAGS_BASE -D_ARCH_CORTEXM0_STM32 ${CLOCK_FREQ} -mcpu=cortex-m0 -mthumb ${OPT_OPTIMIZATION} -c)
list(APPEND CXXFLAGS_BASE -D_ARCH_CORTEXM0_STM32 ${CLOCK_FREQ} -mcpu=cortex-m0 -mthumb ${OPT_OPTIMIZATION} -c)
list(APPEND LFLAGS_BASE -mcpu=cortex-m0 -mthumb -Wl,--gc-sections,-Map,main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib)
