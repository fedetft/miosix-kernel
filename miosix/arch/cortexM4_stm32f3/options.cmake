# Select architecture specific files
list(APPEND ARCH_SRC
        ${ARCH_ALL_COMMON_PATH}/core/interrupts_cortexMx.cpp
        ${ARCH_ALL_COMMON_PATH}/drivers/serial_stm32.cpp
        ${ARCH_PATH}/interfaces-impl/portability.cpp
        ${ARCH_PATH}/interfaces-impl/gpio_impl.cpp
        ${ARCH_PATH}/interfaces-impl/delays.cpp
        ${ARCH_ALL_COMMON_PATH}/CMSIS/Device/ST/STM32F3xx/Source/Templates/system_stm32f3xx.c)

# Select appropriate compiler flags for both ASM / C / C++ / linker
list(APPEND AFLAGS_BASE -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)
list(APPEND CFLAGS_BASE -D_ARCH_CORTEXM4_STM32F3 ${CLOCK_FREQ} ${XRAM} -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ${OPT_OPTIMIZATION} -c)
list(APPEND CXXFLAGS_BASE -D_ARCH_CORTEXM4_STM32F3 ${CLOCK_FREQ} ${XRAM} -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -c)
list(APPEND LFLAGS_BASE -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections,-Map,main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib)
