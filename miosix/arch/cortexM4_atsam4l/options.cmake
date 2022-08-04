# Select architecture specific files
list(APPEND ARCH_SRC
        ${ARCH_ALL_COMMON_PATH}/core/interrupts_cortexMx.cpp
        ${ARCH_ALL_COMMON_PATH}/drivers/serial_atsam4l.cpp
        ${ARCH_PATH}/drivers/clock.cpp
        ${ARCH_PATH}/drivers/lcd.cpp
        ${ARCH_PATH}/interfaces-impl/portability.cpp
        ${ARCH_PATH}/interfaces-impl/gpio_impl.cpp
        ${ARCH_PATH}/interfaces-impl/delays.cpp)

# Select appropriate compiler flags for both ASM / C / C++ / linker.
# NOTE: although the atsam4l are cortex M4, they have no FPU, so the closest
# multilib we have is cortex M3, and that's basically what they are.
list(APPEND AFLAGS_BASE -mcpu=cortex-m3 -mthumb)
list(APPEND CFLAGS_BASE -D_ARCH_CORTEXM4_ATSAM4L ${CLOCK_FREQ} ${XRAM} -mcpu=cortex-m3 -mthumb ${OPT_OPTIMIZATION} -c)
list(APPEND CXXFLAGS_BASE -D_ARCH_CORTEXM4_ATSAM4L ${CLOCK_FREQ} ${XRAM} -mcpu=cortex-m3 -mthumb ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -c)
list(APPEND LFLAGS_BASE -mcpu=cortex-m3 -mthumb -Wl,--gc-sections,-Map,main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib)
