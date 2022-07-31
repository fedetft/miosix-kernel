# Select architecture specific files
list(APPEND ARCH_SRC
        ${ARCH_ALL_COMMON_PATH}/core/interrupts_cortexMx.cpp
        ${ARCH_ALL_COMMON_PATH}/arch/common/drivers/serial_efm32.cpp
        ${ARCH_PATH}/common/interfaces-impl/portability.cpp
        ${ARCH_PATH}/common/interfaces-impl/gpio_impl.cpp
        ${ARCH_PATH}/common/interfaces-impl/delays.cpp
        ${ARCH_ALL_COMMON_PATH}/CMSIS/Device/SiliconLabs/EFM32GG/Source/system_efm32gg.c)

# Select appropriate compiler flags for both ASM / C / C++ /linker
list(APPEND AFLAGS_BASE -mcpu=cortex-m3 -mthumb)
list(APPEND CFLAGS_BASE -D_ARCH_CORTEXM3_EFM32GG ${CLOCK_FREQ} -mcpu=cortex-m3 -mthumb ${OPT_OPTIMIZATION} -c)
list(APPEND CXXFLAGS_BASE -D_ARCH_CORTEXM3_EFM32GG ${CLOCK_FREQ} ${OPT_EXCEPT} -mcpu=cortex-m3 -mthumb ${OPT_OPTIMIZATION} -c)
list(APPEND LFLAGS_BASE -mcpu=cortex-m3 -mthumb -Wl,--gc-sections,-Map=main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib)
