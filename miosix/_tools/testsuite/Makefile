##
## Makefile for Miosix embedded OS
##

## Path to kernel/config directories (edited by init_project_out_of_git_repo.pl)
KPATH := ../..
CONFPATH := ../..
MAKEFILE_VERSION := 1.14
include $(KPATH)/Makefile.kcommon

##
## List here your source files (both .s, .c and .cpp)
##
SRC := testsuite.cpp

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
SUBDIRS += test_process test_execve test_global_dtor_ctor

##
## Attach a romfs filesystem image after the kernel
##
ROMFS_DIR := testsuite_romfs

all: main $(if $(ROMFS_DIR), image)

clean: clean-recursive
	-rm -f $(OBJ) $(OBJ:.o=.d) main.elf main.hex main.bin main.map

main: main.hex main.bin main.elf
	$(Q)$(SZ) main.elf

main.elf: $(OBJ) all-recursive
	$(ECHO) "[LD  ] main.elf"
	$(Q)$(CXX) $(LFLAGS) -o main.elf $(OBJ) $(LINK_LIBS)

-include $(OBJ:.o=.d)
