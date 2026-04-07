##
## Makefile for board efm32gg332f1024_wandstem
## Included by Makefile.inc based on the board currently selected
##

## Base directory with else header files for this board
CHIP_INC := arch/chip/efm32gg

## Select architecture specific files
## These are the files in arch/<arch name>/<board name>
BOARD_SRC :=                                                 \
$(BOARD_INC)/interfaces-impl/bsp.cpp                        \
$(BOARD_INC)/interfaces-impl/spi.cpp                        \
$(BOARD_INC)/interfaces-impl/power_manager.cpp              \
$(BOARD_INC)/interfaces-impl/os_timer.cpp                   \
$(BOARD_INC)/interfaces-impl/timer_interface.cpp            \
$(BOARD_INC)/interfaces-impl/rtc.cpp                        \
$(BOARD_INC)/interfaces-impl/hrtb.cpp                       \
$(BOARD_INC)/interfaces-impl/gpio_timer.cpp                 \
$(BOARD_INC)/interfaces-impl/transceiver_timer.cpp          \
$(BOARD_INC)/interfaces-impl/gpioirq.cpp                    \
$(BOARD_INC)/interfaces-impl/transceiver.cpp                \
$(BOARD_INC)/interfaces-impl/flopsync_vht.cpp               \
$(BOARD_INC)/interfaces-impl/vht.cpp                        \
$(BOARD_INC)/interfaces-impl/virtual_clock.cpp

## Add a #define to allow querying board name
BOARD_CFLAGS   := -D_BOARD_WANDSTEM
BOARD_CXXFLAGS := -D_BOARD_WANDSTEM

## Select programmer command line
## This is the program that is invoked when the user types
## 'make program'
## The command must provide a way to program the board, or print an
## error message saying that 'make program' is not supported for that
## board.
PROG ?= wandstem-flash -m u -f $(if $(ROMFS_DIR), image.bin, main.bin)
