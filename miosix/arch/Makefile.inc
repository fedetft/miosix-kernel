##
## Makefile for Miosix embedded OS
##
## This file contains the options required by the build system to build
## Miosix on various target architectures. All options start with OPT_
## to be easily recognizable.
## All architecture specific build code is grouped at the end of this file.
##

##
## Check that the top-level makefile (which includes this file) has the correct
## version
##
ifneq ($(MAKEFILE_VERSION),1.16)
    $(info You are using an incompatible makefile. Make sure it matches \
      the one distributed with the current version of the kernel)
    $(error Error)
endif

##
## Include user-specified configuration. This file must exist.
## This makefile should define at least OPT_BOARD
##
include $(CONFPATH)/config/Makefile.inc

## Check that a board was indeed specified by the user and define BOARD_INC
## based on that
ifeq ($(OPT_BOARD),)
    $(info Error: no board specified in $(CONFPATH)/config/Makefile.inc)
    $(error Error)
endif
BOARD_INC := arch/board/$(OPT_BOARD)

##
## Include user-specified board configuration
## This include might not exist for some boards
##
-include $(CONFPATH)/config/board/$(OPT_BOARD)/Makefile.inc

##
## Include the makefile for the selected board
##
include $(KPATH)/$(BOARD_INC)/Makefile.inc

##
## Include the makefile for the selected chip
## This will define PREFIX
##
include $(KPATH)/$(CHIP_INC)/Makefile.inc

##
## Assemble list of architecture dependent source files
##
ARCH_SRC := $(BOARD_SRC) $(CHIP_SRC)

##
## Default optimization level
##
OPT_OPTIMIZATION ?= -O2

##
## Assemble compilation flags (shared between kernel and processes)
##
AFLAGS_BASE   := $(BOARD_AFLAGS) $(CHIP_AFLAGS) $(AFLAGS_BASE)
CFLAGS_BASE   := -D_MIOSIX_BOARDNAME=\"$(OPT_BOARD)\" -D_DEFAULT_SOURCE=1 \
                 $(BOARD_CFLAGS) $(CHIP_CFLAGS) $(OPT_OPTIMIZATION)       \
                 $(CFLAGS_BASE)
CXXFLAGS_BASE := -D_MIOSIX_BOARDNAME=\"$(OPT_BOARD)\" -D_DEFAULT_SOURCE=1 \
                 $(BOARD_CXXFLAGS) $(CHIP_CXXFLAGS) $(OPT_OPTIMIZATION)   \
                 $(OPT_EXCEPT) $(CXXFLAGS_BASE)
LFLAGS_BASE   := $(BOARD_LFLAGS) $(CHIP_LFLAGS)                \
                 -Wl,--gc-sections,-Map,main.map               \
                 -Wl,-T$(KPATH)/$(LINKER_SCRIPT) $(OPT_EXCEPT) \
                 $(OPT_OPTIMIZATION) -nostdlib

## From compiler prefix form the name of the compiler and other tools
CC     := $(PREFIX)gcc
CXX    := $(PREFIX)g++
LD     := $(PREFIX)ld
AR     := $(PREFIX)ar
AS     := $(PREFIX)as
CP     := $(PREFIX)objcopy
OD     := $(PREFIX)objdump
SZ     := $(PREFIX)size
STRIP  := $(PREFIX)strip

ifeq ("$(VERBOSE)","1")
Q :=
ECHO := @true
else
Q := @
ECHO := @echo
endif
