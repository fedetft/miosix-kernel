##
## Makefile for Miosix np embedded OS
## TFT:Terraneo Federico Technlogies
## This makefile builds the whole kernel
##
MAKEFILE_VERSION := 1.01
include config/Makefile.inc

## List of all Miosix OS source files that have no special requirements
## and that must be built for each architecture (both .c and .cpp)
## These files will end up in libmiosix.a
SRC :=                                                                     \
kernel/kernel.cpp                                                          \
kernel/sync.cpp                                                            \
kernel/error.cpp                                                           \
kernel/pthread.cpp                                                         \
kernel/unistd.cpp                                                          \
kernel/stage_2_boot.cpp                                                    \
kernel/syscalls.cpp                                                        \
kernel/scheduler/priority/priority_scheduler.cpp                           \
kernel/scheduler/control/control_scheduler.cpp                             \
kernel/scheduler/edf/edf_scheduler.cpp                                     \
kernel/filesystem/filesystem.cpp                                           \
kernel/filesystem/ff.c                                                     \
kernel/filesystem/diskio.cpp                                               \
util/util.cpp                                                              \
util/version.cpp                                                           \
util/crc16.cpp                                                             \
util/lcd44780.cpp

## Add the architecture dependand sources to the list of files to build.
## ARCH_SRC will contain different source files depending on which
## architecture/board is selected in config/Makefile.inc
SRC += $(ARCH_SRC)

## Replaces both "foo.cpp"-->"foo.o" and "foo.c"-->"foo.o"
OBJ := $(addsuffix .o, $(basename $(SRC)))

## Includes the miosix base directory for C/C++
CXXFLAGS := $(CXXFLAGS_BASE) -I. -Iarch/common -I$(ARCH_INC) -I$(BOARD_INC)
CFLAGS   :=  $(CFLAGS_BASE)  -I. -Iarch/common -I$(ARCH_INC) -I$(BOARD_INC)
AFLAGS   :=  $(AFLAGS_BASE)

## Build libmiosix.a and stage_1_boot.o (whose path is in BOOT_FILE)
## The file stage_1_boot.o is compiled separately because
## it must not end up in libmiosix.a
all: $(OBJ) $(BOOT_FILE)
	$(AR) rcs libmiosix.a $(OBJ)

clean:
	rm $(OBJ) $(BOOT_FILE) libmiosix.a

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@
