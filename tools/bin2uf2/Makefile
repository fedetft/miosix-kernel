##
## Makefile template for Miosix embedded OS kernel-side applications
##

## Path to the Miosix kernel root (miosix-kernel/miosix)
KPATH := miosix

## Path to the Miosix config root, containing:
##  - config/Makefile.inc
##  - config/miosix_config.h (optional)
##  - config/board/<board name>/board_config.h (optional)
## Leave it as $(KPATH) to use the default settings built in to miosix. Any
## missing file is replaced by the defaults as well.
##   To change the config, copy and paste the miosix/config directory to
## your project root and change this path to the project root (typically ".")
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
SUBDIRS +=

##
## Attach a romfs filesystem image after the kernel
##
ROMFS_DIR :=

all: $(if $(ROMFS_DIR), image, main)

main: $(OBJ) all-recursive $(KPATH)/../tools/bin2uf2/bin2uf2
	$(ECHO) "[LD  ] main.elf"
	$(Q)$(CXX) $(LFLAGS) -o main.elf $(OBJ) $(LINK_LIBS)
	$(ECHO) "[CP  ] main.hex"
	$(Q)$(CP) -O ihex   main.elf main.hex
	$(ECHO) "[CP  ] main.bin"
	$(Q)$(CP) -O binary main.elf main.bin
	$(ECHO) "[UF2 ] main.uf2"
	$(KPATH)/../tools/bin2uf2/bin2uf2 -s 0x10000000 -f 0xe48bff56 -p main.bin -o main.uf2
	$(Q)$(SZ) main.elf

$(KPATH)/../tools/bin2uf2/bin2uf2:
	$(ECHO) "[HOST] bin2uf2"
	$(Q)mkdir -p $(KPATH)/../tools/bin2uf2/build
	$(Q)cd $(KPATH)/../tools/bin2uf2/build             \
	  && cmake --log-level=ERROR .. && $(MAKE) -s

clean: clean-recursive
	$(Q)rm -f $(OBJ) $(OBJ:.o=.d) main.elf main.hex main.bin main.map
	$(Q)rm -f main.uf2 image.uf2 $(KPATH)/../tools/bin2uf2/bin2uf2
	$(Q)rm -rf $(KPATH)/../tools)/bin2uf2/build

-include $(OBJ:.o=.d)
