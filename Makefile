##
## Makefile for Miosix np embedded OS
## TFT:Terraneo Federico Technlogies
##
include miosix/config/Makefile.inc

##
## List here subdirectories which contains makefiles
##
SUBDIRS := miosix

##
## List here your source files (both .s, .c and .cpp)
##
SRC :=                                  \
x.cpp

##
## List here additional static libraries with relative path
##
LIBS :=

##
## List here additional include directories (in the form -Iinclude_dir)
##
INCLUDE_DIRS :=

##############################################################################
## You should not need to modify anything below                             ##
##############################################################################

## Replaces both "foo.cpp"-->"foo.o" and "foo.c"-->"foo.o"
OBJ := $(addsuffix .o, $(basename $(SRC)))

## Includes the miosix base directory for C/C++
CXXFLAGS  := $(CXXFLAGS_BASE) -I. -I./miosix -I./miosix/$(ARCH_INC) \
             -I./miosix/$(BOARD_INC) $(INCLUDE_DIRS)
CFLAGS    := $(CFLAGS_BASE)   -I. -I./miosix -I./miosix/$(ARCH_INC) \
             -I./miosix/$(BOARD_INC) $(INCLUDE_DIRS)
AFLAGS    := $(AFLAGS_BASE)
LFLAGS    := $(LFLAGS_BASE)

LINK_LIBS := $(LIBS) -Wl,--start-group -L./miosix -lmiosix -lstdc++ -lc -lm \
    -lg -lgcc -Wl,--end-group

all: all-recursive main

clean: clean-recursive clean-topdir

program:
	$(PROGRAM_CMDLINE)

all-recursive:
	@for i in $(SUBDIRS); do  \
		$(MAKE) -C $$i FOLDER="$(FOLDER) $$i/" || exit 1;  \
	done

clean-recursive:
	@for i in $(SUBDIRS); do  \
		$(MAKE) -C $$i FOLDER="$(FOLDER) $$i/" clean  || exit 1;  \
	done

clean-topdir:
	-rm $(OBJ) main.elf main.hex main.bin main.map

main: main.elf
	$(CP) -O ihex   main.elf main.hex
	$(CP) -O binary main.elf main.bin
	$(SZ) main.elf

main.elf: $(OBJ) miosix/libmiosix.a
	@ echo "linking"
	$(CXX) $(LFLAGS) -o main.elf $(OBJ) miosix/$(BOOT_FILE) \
	miosix/kernel/syscalls.o $(LINK_LIBS)

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@
