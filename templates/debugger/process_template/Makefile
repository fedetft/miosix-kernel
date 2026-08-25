##
## Makefile for writing processes for the Miosix embedded OS
##

## KPATH and CONFPATH can be specified here or forwarded by the parent makefile
KPATH := ../../../miosix
CONFPATH := $(KPATH)
MAKEFILE_VERSION := 3.01
include $(KPATH)/Makefile.pcommon

# RomFS output directory
BIN := ../romfs/hello
# Process' main
SRC := main.cpp

all: $(OBJ)
	$(ECHO) "[LD  ] $(BIN)"
	$(Q)$(CXX)    $(LFLAGS) -o $(BIN) $(OBJ) $(LINK_LIBS) -g -fno-omit-frame-pointer

	# Copies the binary with debug options into the ./bin directory
	mkdir -p bin
	$(Q)cp        $(BIN) ./bin/$(shell basename $(BIN))

	$(Q)$(SZ)     $(BIN)
	$(Q)$(STRIP)  $(BIN)
	echo $(STRIP)
	$(Q)$(POSTLD) $(BIN) --ramsize=16384 --stacksize=2048 --strip-sectheader

clean:
	$(Q)rm -f $(OBJ) $(OBJ:.o=.d) $(BIN) $(notdir $(BIN)).map

-include $(OBJ:.o=.d)
