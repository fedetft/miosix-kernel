##
## Makefile for Miosix embedded OS
##
## This file contains the options required by the build system to build
## Miosix on various target architectures. All options start with OPT_
## to be easily recognizable.
## All architecture specific build code is grouped at the end of this file.
##

## Include all user-specified configuration, if it exists
-include $(CONFPATH)/config/Makefile.inc

ifneq ($(MAKEFILE_VERSION),1.16)
    $(info You are using an incompatible makefile. Make sure it matches \
      the one distributed with the current version of the kernel)
    $(error Error)
endif

## Check that a board was indeed specified by the user
ifeq (x$(OPT_BOARD),x)
    $(info Error: no board specified in $(CONFPATH)/config/Makefile.inc)
    $(error Error)
endif

## 
## Default for the optimization level
##
OPT_OPTIMIZATION ?= -O2

##
## Add required C/CXX flags for the kernel
##
CFLAGS_BASE   := -D_MIOSIX_BOARDNAME=\"$(OPT_BOARD)\" -D_DEFAULT_SOURCE=1 \
                 $(CFLAGS_BASE)
CXXFLAGS_BASE := -D_MIOSIX_BOARDNAME=\"$(OPT_BOARD)\" -D_DEFAULT_SOURCE=1 \
                 $(CFLAGS_BASE)

##
## Include the makefile for the selected board
## This will in turn include the makefile for the chip
##
include arch/board/$(OPT_BOARD)/Makefile.inc

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
