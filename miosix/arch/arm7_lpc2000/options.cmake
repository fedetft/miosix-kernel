# Select architecture specific files
list(APPEND ARCH_SRC
        ${ARCH_ALL_COMMON_PATH}/core/interrupts_arm7.cpp
        ${ARCH_ALL_COMMON_PATH}/drivers/serial_lpc2000.cpp)

# Select programmer command line.
# This is the program that is invoked when the user types 'make program'
# The command must provide a way to program the board, or print an error
# message saying that 'make program' is not supported for that board.

set(PROGRAM_CMDLINE "lpc21isp -verify main.hex /dev/ttyUSB0 115200 14746")

## Select appropriate compiler flags for both ASM / C / C++ / linker
list(APPEND AFLAGS_BASE -mcpu=arm7tdmi -mapcs-32 -mfloat-abi=soft)
list(APPEND CFLAGS_BASE -D_ARCH_ARM7_LPC2000 -mcpu=arm7tdmi ${OPT_OPTIMIZATION} -c)
list(APPEND CXXFLAGS_BASE -D_ARCH_ARM7_LPC2000 -mcpu=arm7tdmi ${OPT_OPTIMIZATION} ${OPT_EXCEPT} -c)
list(APPEND LFLAGS_BASE -mcpu=arm7tdmi -Wl,--gc-sections,-Map=main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib)
