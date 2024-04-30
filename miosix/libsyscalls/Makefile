##
## Makefile for libsyscalls, the library with userspace syscalls for processes
## This makefile builds libsyscalls.a
##

## KPATH and CONFPATH should be forwarded by the parent makefile
MAKEFILE_VERSION := 1.13
include Makefile.pcommon

SRC := crt0.s crt1.cpp

all: $(OBJ)
	$(ECHO) "[AR  ] libsyscalls.a"
	$(Q)$(AR) rcs libsyscalls.a $(OBJ)

clean:
	-rm -f $(OBJ) $(OBJ:.o=.d) libsyscalls.a

-include $(OBJ:.o=.d)
