##
## CMakeLists.txt for board hd2 (Ailunce HD2 — HR_C7000 / CK803S)
##

# Directory with header files for this board's chip
set(MIOSIX_CHIP_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/chip/hr_c7000)

# Linker script options. Each script may be followed by the compiler options it
# requires; those are added to the command line when the script is selected.
set(MIOSIX_LINKER_SCRIPT_LIST
    hd2.ld
)
set(MIOSIX_DEFAULT_LINKER_SCRIPT hd2.ld)

# Board-specific sources
set(MIOSIX_BOARD_SRC
    ${MIOSIX_BOARD_INC}/interfaces-impl/bsp.cpp
)

# Allow querying the board name
set(MIOSIX_BOARD_C_FLAGS -D_BOARD_HD2)
set(MIOSIX_BOARD_CXX_FLAGS -D_BOARD_HD2)

# Flashing is done locally on the Mac via the YMODEM/DFU loader (rtx.py), not a
# generic programmer, so no PROGRAM_CMDLINE here.
