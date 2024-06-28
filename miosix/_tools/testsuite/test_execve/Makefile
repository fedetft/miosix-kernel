##
## Makefile for writing processes for the Miosix embedded OS
##

## KPATH and CONFPATH can be specified here or forwarded by the parent makefile
MAKEFILE_VERSION := 1.15
include $(KPATH)/libsyscalls/Makefile.pcommon

BIN := ../testsuite_romfs/test_execve
SRC := main.cpp

all: $(OBJ)
	$(ECHO) "[LD  ] $(BIN)"
	$(Q)$(CXX)    $(LFLAGS) -o $(BIN) $(OBJ) $(LINK_LIBS)
	$(Q)$(SZ)     $(BIN)
	$(Q)$(STRIP)  $(BIN)
	$(Q)$(POSTLD) $(BIN) --ramsize=16384 --stacksize=2048 --strip-sectheader

clean:
	-rm -f $(OBJ) $(OBJ:.o=.d) $(BIN) $(notdir $(BIN)).map

-include $(OBJ:.o=.d)
