##
## Makefile for libsyscalls, the library with userspace syscalls for processes
## This makefile builds libsyscalls.a
##

## KPATH and CONFPATH should be forwarded by the parent makefile
MAKEFILE_VERSION := 1.15
include Makefile.pcommon

SRC := crt0.s crt1.cpp memoryprofiling.cpp

## Process code shouldn't include kernel headers, but memoryprofiling.cpp
## needs to include miosix_settings.h. For this reason we add the required
## include paths only here and not in Makefile.pcommon
CXXFLAGS += -I$(CONFPATH) -I$(CONFPATH)/config/$(BOARD_INC) -I$(KPATH)/$(ARCH_INC)

all: $(OBJ)
	$(ECHO) "[AR  ] libsyscalls.a"
	$(Q)$(AR) rcs libsyscalls.a $(OBJ)

clean:
	$(Q)rm -f $(OBJ) $(OBJ:.o=.d) libsyscalls.a

-include $(OBJ:.o=.d)
