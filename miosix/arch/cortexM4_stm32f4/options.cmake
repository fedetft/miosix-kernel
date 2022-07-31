# Select architecture specific files
list(APPEND ARCH_SRC
        ${ARCH_ALL_COMMON_PATH}/core/interrupts_cortexMx.cpp
        ${ARCH_ALL_COMMON_PATH}/core/mpu_cortexMx.cpp
        ${ARCH_ALL_COMMON_PATH}/drivers/serial_stm32.cpp
        ${ARCH_ALL_COMMON_PATH}/drivers/dcc.cpp
        ${ARCH_PATH}/common/interfaces-impl/portability.cpp
        ${ARCH_PATH}/common/interfaces-impl/delays.cpp
        ${ARCH_PATH}/common/interfaces-impl/gpio_impl.cpp
        ${ARCH_ALL_COMMON_PATH}/drivers/sd_stm32f2_f4.cpp
        ${ARCH_ALL_COMMON_PATH}/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c)

# Select appropriate compiler flags for both ASM / C / C++ / linker
list(APPEND AFLAGS_BASE -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)
list(APPEND CFLAGS_BASE -D_ARCH_CORTEXM4_STM32F4 ${CLOCK_FREQ} ${XRAM} ${SRAM_BOOT} -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ${OPT_OPTIMIZATION} -c)
list(APPEND CXXFLAGS_BASE -D_ARCH_CORTEXM4_STM32F4 ${CLOCK_FREQ} ${XRAM} ${SRAM_BOOT} -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -c)
list(APPEND LFLAGS_BASE -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections,-Map=main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib)
