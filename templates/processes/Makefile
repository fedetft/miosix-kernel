##
## Makefile template for Miosix embedded OS kernel-side applications
##

## Path to the Miosix kernel root (miosix-kernel/miosix)
KPATH := ../../miosix

## Path to the Miosix config root, containing:
##  - config/Makefile.inc
##  - config/miosix_config.h (optional)
##  - config/board/<board name>/board_config.h (optional)
## For the processes example the configuration needs to be changed in order to
## enable the define WITH_PROCESSES and choose an appropriate linker file for
## the board.
CONFPATH := $(KPATH)

## When using CONFPATH := $(KPATH) you can choose the board here. You can also
## override other variables defined by Makefile.inc.
# OPT_BOARD := [options in miosix-kernel/miosix/config/Makefile.inc]

## Include the Miosix common makefile for kernel-side applications
MAKEFILE_VERSION := 1.16
include $(KPATH)/Makefile.kcommon

##
## List here your source files (both .s, .c and .cpp)
##
SRC := main.cpp

##
## List here additional include directories (in the form -Iinclude_dir)
##
INCLUDE_DIRS :=

##
## List here additional static libraries with relative path
##
LIBS :=

##
## List here subdirectories which contains makefiles
##
# We add the directory for our subprocess
SUBDIRS += process_template

##
## Attach a romfs filesystem image after the kernel
##
# We specify a romfs directory; this directory needs to already exist
ROMFS_DIR := romfs

all: $(if $(ROMFS_DIR), image, main)

main: $(OBJ) all-recursive
	$(ECHO) "[LD  ] main.elf"
	$(Q)$(CXX) $(LFLAGS) -o main.elf $(OBJ) $(LINK_LIBS)
	$(ECHO) "[CP  ] main.hex"
	$(Q)$(CP) -O ihex   main.elf main.hex
	$(ECHO) "[CP  ] main.bin"
	$(Q)$(CP) -O binary main.elf main.bin
	$(Q)$(SZ) main.elf

clean: clean-recursive
	$(Q)rm -f $(OBJ) $(OBJ:.o=.d) main.elf main.hex main.bin main.map

-include $(OBJ:.o=.d)
