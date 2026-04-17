##
## CMakeLists.txt for board stm32f765ii_marco_ram_board
##

# Directory with header files for this board
set(MIOSIX_CHIP_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/chip/stm32f7)

# Set the path to the multilib directory for this architecture (used by clang)
set(MIOSIX_MULTILIB_PATH thumb/v7e-m+dp/hard)

# The stm32f767 has the double precision FPU, so we will build for m7
set(MIOSIX_CPU_FLAGS -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16)

# Linker script options
# Each script in the list may be followed by the compiler options it requires
# These options are automatically added to the C/CXX compiler command line when
# that linker script is selected
set(MIOSIX_LINKER_SCRIPT_LIST
    # Linker script type
    # 1)   - .text                      internal Flash
    #      - IRQ stack                  internal SRAM
    #      - .data, .bss                internal SRAM
    #      - Heap (and thread stacks)   internal SRAM
    #    The most common choice, available for all microcontrollers
    unikernel.ld
    # 2)   - .text                      internal Flash
    #      - IRQ stack                  internal SRAM
    #      - .data, .bss                internal SRAM
    #      - Heap (and thread stacks)   external SDRAM (bank 1)
    #    For application with small static buffers. Only uses SDRAM bank 1.
    unikernel-xram-heap.ld
    # 3)   - .text                      internal Flash
    #      - IRQ stack                  internal SRAM
    #      - .data, .bss                external SDRAM (bank 1)
    #      - Heap (and thread stacks)   external SDRAM (bank 1)
    #    For application with large static buffers. Only uses SDRAM bank 1.
    unikernel-xram.ld
    # 4)   - .text                      internal Flash
    #      - IRQ stack                  internal SRAM
    #      - .data, .bss                internal SRAM
    #      - Heap (and thread stacks)   internal SRAM
    #      - Process Pool               internal SRAM
    processes.ld            -DWITH_PROCESSES
    # 5) stm32_2m+384k_xram_256M_processes.ld
    #      - .text                      internal Flash
    #      - IRQ stack                  internal SRAM
    #      - .data, .bss                internal SRAM
    #      - Heap (and thread stacks)   internal SRAM
    #      - Process Pool               external SDRAM (bank 1)
    processes-xram.ld       -DWITH_PROCESSES
)
set(MIOSIX_DEFAULT_LINKER_SCRIPT unikernel-xram-heap.ld)

# Select architecture specific files
set(MIOSIX_BOARD_SRC
    ${MIOSIX_BOARD_INC}/boot.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/bsp.cpp
)

# Add a #define to allow querying board name
set(MIOSIX_BOARD_C_FLAGS -D_BOARD_STM32F765II_MARCO_RAM_BOARD)
set(MIOSIX_BOARD_CXX_FLAGS -D_BOARD_STM32F765II_MARCO_RAM_BOARD)

# Specify a custom flash command
# This is the program that is invoked when the program-<target_name> target is
# built. Use <binary> or <hex> as placeolders, they will be replaced by the
# build systems with the binary or hex file path repectively.
# If a command is not specified, the build system will fallback to st-flash
set(PROGRAM_CMDLINE openocd -f "${MIOSIX_BOARD_INC}/openocd.cfg" -c "program \"<binary>\" 0x8000000 verify reset" -c shutdown)
