##
## Makefile for Miosix embedded OS
## This makefile builds the whole kernel
##

## KPATH and CONFPATH are forwarded by the parent Makefile
MAKEFILE_VERSION := 1.15
include $(KPATH)/Makefile.kcommon

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
kernel/timeconversion.cpp                                                  \
kernel/intrusive.cpp                                                       \
kernel/cpu_time_counter.cpp                                                \
kernel/scheduler/priority/priority_scheduler.cpp                           \
kernel/scheduler/control/control_scheduler.cpp                             \
kernel/scheduler/edf/edf_scheduler.cpp                                     \
filesystem/file_access.cpp                                                 \
filesystem/file.cpp                                                        \
filesystem/path.cpp                                                        \
filesystem/stringpart.cpp                                                  \
filesystem/pipe/pipe.cpp                                                   \
filesystem/console/console_device.cpp                                      \
filesystem/mountpointfs/mountpointfs.cpp                                   \
filesystem/devfs/devfs.cpp                                                 \
filesystem/fat32/fat32.cpp                                                 \
filesystem/fat32/ff.cpp                                                    \
filesystem/fat32/diskio.cpp                                                \
filesystem/fat32/wtoupper.cpp                                              \
filesystem/fat32/ccsbcs.cpp                                                \
filesystem/littlefs/lfs_miosix.cpp                                         \
filesystem/littlefs/lfs.c                                                  \
filesystem/littlefs/lfs_util.c                                             \
filesystem/romfs/romfs.cpp                                                 \
stdlib_integration/libc_integration.cpp                                    \
stdlib_integration/libstdcpp_integration.cpp                               \
e20/e20.cpp                                                                \
e20/unmember.cpp                                                           \
util/util.cpp                                                              \
util/unicode.cpp                                                           \
util/version.cpp                                                           \
util/crc16.cpp                                                             \
util/lcd44780.cpp

## Add the architecture dependand sources to the list of files to build.
## ARCH_SRC will contain different source files depending on which
## architecture/board is selected in config/Makefile.inc
SRC += $(ARCH_SRC)

CFLAGS   += -DCOMPILING_MIOSIX
CXXFLAGS += -DCOMPILING_MIOSIX

all: $(OBJ)
	$(ECHO) "[PERL] Checking global objects"
	$(Q)perl _tools/kernel_global_objects.pl $(OBJ)
	$(ECHO) "[AR  ] libmiosix.a"
	$(Q)$(AR) rcs libmiosix.a $(OBJ)

clean:
	$(Q)rm -f $(OBJ) $(OBJ:.o=.d) libmiosix.a

-include $(OBJ:.o=.d)
