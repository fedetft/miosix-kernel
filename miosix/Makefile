##
## Makefile for Miosix embedded OS
## This makefile builds the whole kernel
##
MAKEFILE_VERSION := 1.04
## KPATH and CONFPATH are forwarded by the parent Makefile
include $(CONFPATH)/config/Makefile.inc

## List of all Miosix OS source files that have no special requirements
## and that must be built for each architecture (both .c and .cpp)
## These files will end up in libmiosix.a
SRC :=                                                                     \
kernel/kernel.cpp                                                          \
kernel/sync.cpp                                                            \
kernel/error.cpp                                                           \
kernel/pthread.cpp                                                         \
kernel/stage_2_boot.cpp                                                    \
kernel/elf_program.cpp                                                     \
kernel/process.cpp                                                         \
kernel/process_pool.cpp                                                    \
kernel/SystemMap.cpp                                                       \
kernel/scheduler/priority/priority_scheduler.cpp                           \
kernel/scheduler/control/control_scheduler.cpp                             \
kernel/scheduler/edf/edf_scheduler.cpp                                     \
filesystem/file_access.cpp                                                 \
filesystem/file.cpp                                                        \
filesystem/stringpart.cpp                                                  \
filesystem/console/console_device.cpp                                      \
filesystem/mountpointfs/mountpointfs.cpp                                   \
filesystem/devfs/devfs.cpp                                                 \
filesystem/fat32/fat32.cpp                                                 \
filesystem/fat32/ff.cpp                                                    \
filesystem/fat32/diskio.cpp                                                \
filesystem/fat32/wtoupper.cpp                                              \
filesystem/fat32/ccsbcs.cpp                                                \
stdlib_integration/libc_integration.cpp                                    \
stdlib_integration/libstdcpp_integration.cpp                               \
e20/e20.cpp                                                                \
util/util.cpp                                                              \
util/unicode.cpp                                                           \
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
## Always include CONFPATH first, as it overrides the config file location
CXXFLAGS := $(CXXFLAGS_BASE) -I$(CONFPATH) -I$(CONFPATH)/config/$(BOARD_INC)  \
            -I. -Iarch/common -I$(ARCH_INC) -I$(BOARD_INC) -DCOMPILING_MIOSIX
CFLAGS   := $(CFLAGS_BASE)   -I$(CONFPATH) -I$(CONFPATH)/config/$(BOARD_INC)  \
            -I. -Iarch/common -I$(ARCH_INC) -I$(BOARD_INC) -DCOMPILING_MIOSIX
AFLAGS   := $(AFLAGS_BASE)
DFLAGS    := -MMD -MP

## Build libmiosix.a and stage_1_boot.o (whose path is in BOOT_FILE)
## The file stage_1_boot.o is compiled separately because
## it must not end up in libmiosix.a
all: $(OBJ) $(BOOT_FILE)
	perl _tools/check_global_objects.pl $(OBJ)
	$(AR) rcs libmiosix.a $(OBJ)

clean:
	-rm -f $(OBJ) $(BOOT_FILE) libmiosix.a $(OBJ:.o=.d)
	-rm -f $(BOOT_FILE:.o=.d)

%.o: %.s
	$(AS)  $(AFLAGS) $< -o $@

%.o : %.c
	$(CC)  $(DFLAGS) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CXX) $(DFLAGS) $(CXXFLAGS) $< -o $@

#pull in dependecy info for existing .o files
-include $(OBJ:.o=.d)
