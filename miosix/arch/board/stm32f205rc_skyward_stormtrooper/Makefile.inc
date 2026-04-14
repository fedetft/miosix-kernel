##
## Makefile for board stm32f205rc_skyward_stormtrooper
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
CHIP_INC := arch/chip/stm32f2

## Select linker script
LINKER_SCRIPT := unikernel.ld

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
BOARD_SRC :=                         \
arch/drivers/stm32f2_f4_i2c.cpp      \
$(BOARD_INC)/boot.cpp                \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
BOARD_CFLAGS   := -D_BOARD_STM32F205RC_SKYWARD_STORMTROOPER
BOARD_CXXFLAGS := -D_BOARD_STM32F205RC_SKYWARD_STORMTROOPER
