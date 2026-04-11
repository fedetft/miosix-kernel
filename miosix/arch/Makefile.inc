##
## Makefile for Miosix embedded OS
##
## This file contains the options required by the Miosix build system to
## configure it for various target architectures. User-modifiable options
## are set by the config/Makefile.inc file, which is included from here.
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
## Default optimization level
##

## Kernel
OPT_OPTIMIZATION ?= -O2

## Processes
PROC_OPT_OPTIMIZATION ?= -O2

##
## Define tool names
##

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

## Define the echo tool (this is here to share the definition between processes
## and kernel)
ifeq ("$(VERBOSE)","1")
Q :=
ECHO := @true
else
Q := @
ECHO := @echo
endif
