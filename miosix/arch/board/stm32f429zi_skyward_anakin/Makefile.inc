##
## Makefile for board stm32f429zi_skyward_anakin
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
ARCH_INC := arch/cortexM4_stm32f4/common

## Base directory with header files for this board
BOARD_INC := arch/cortexM4_stm32f4/stm32f429zi_skyward_anakin

## Select linker script
#LINKER_SCRIPT := already selected in board options

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
ARCH_SRC :=                                  \
arch/common/drivers/stm32f2_f4_i2c.cpp       \
arch/common/drivers/stm32_hardware_rng.cpp   \
arch/common/drivers/stm32_sgm.cpp            \
arch/common/drivers/stm32_wd.cpp             \
$(BOARD_INC)/boot.cpp                        \
$(BOARD_INC)/interfaces-impl/bsp.cpp

## Add a #define to allow querying board name
CFLAGS_BASE   += -D_BOARD_STM32F429ZI_SKYWARD_ANAKIN
CXXFLAGS_BASE += -D_BOARD_STM32F429ZI_SKYWARD_ANAKIN

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= stm32flash -w $(if $(ROMFS_DIR), image.bin, main.bin) -v /dev/ttyUSB0

## Include the makefile for the board's chip
include $(ARCH_INC)/Makefile.inc
