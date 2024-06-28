##
## Makefile for Miosix embedded OS
##

## Path to kernel/config directories (edited by init_project_out_of_git_repo.pl)
KPATH := miosix
CONFPATH := $(KPATH)
MAKEFILE_VERSION := 1.15
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

main: $(OBJ) all-recursive $(TOOLS_DIR)/bin2uf2/bin2uf2
	$(ECHO) "[LD  ] main.elf"
	$(Q)$(CXX) $(LFLAGS) -o main.elf $(OBJ) $(LINK_LIBS)
	$(ECHO) "[CP  ] main.hex"
	$(Q)$(CP) -O ihex   main.elf main.hex
	$(ECHO) "[CP  ] main.bin"
	$(Q)$(CP) -O binary main.elf main.bin
	$(ECHO) "[UF2 ] main.uf2"
	$(TOOLS_DIR)/bin2uf2/bin2uf2 -s 0x10000000 -f 0xe48bff56 -p main.bin -o main.uf2
	$(Q)$(SZ) main.elf

$(TOOLS_DIR)/bin2uf2/bin2uf2:
	$(ECHO) "[HOST] bin2uf2"
	$(Q)mkdir -p $(TOOLS_DIR)/bin2uf2/build
	$(Q)cd $(TOOLS_DIR)/bin2uf2/build             \
	  && cmake --log-level=ERROR .. && $(MAKE) -s

clean: clean-recursive
	$(Q)rm -f $(OBJ) $(OBJ:.o=.d) main.elf main.hex main.bin main.map
	$(Q)rm -f main.uf2 image.uf2 $(TOOLS_DIR)/bin2uf2/bin2uf2
	$(Q)rm -rf $(TOOLS_DIR)/bin2uf2/build

-include $(OBJ:.o=.d)
