##
## Makefile for Miosix embedded OS
## This makefile builds the whole kernel
##

project(kernel)
include(config/CMakeLists.txt)

include_directories(
        ${TOOLCHAIN_PATH}/arm-miosix-eabi/include
        ${TOOLCHAIN_PATH}/lib/gcc/arm-miosix-eabi/4.7.3/include
        ${TOOLCHAIN_PATH}/lib/gcc/arm-miosix-eabi/4.7.3/include-fixed
        ${TOOLCHAIN_PATH}/arm-miosix-eabi/include/c++/4.7.3/
        ${TOOLCHAIN_PATH}/arm-miosix-eabi/include/c++/4.7.3/arm-miosix-eabi
        ${TOOLCHAIN_PATH}/arm-miosix-eabi/include/c++/4.7.3/backward
        ${CONFPATH}
        ${CONFPATH}/${BOARD_INC}
        ${KPATH}
        arch/common
        ${ARCH_INC}
        ${BOARD_INC}
        ${INCLUDE_DIRS}
)

## List of all Miosix OS source files that have no special requirements
## and that must be built for each architecture (both .c and .cpp)
## These files will end up in libmiosix.a
set(SOURCES
        kernel/kernel.cpp
        kernel/sync.cpp
        kernel/error.cpp
        kernel/pthread.cpp
        kernel/stage_2_boot.cpp
        kernel/elf_program.cpp
        kernel/process.cpp
        kernel/process_pool.cpp
        kernel/timeconversion.cpp
        kernel/SystemMap.cpp
        kernel/scheduler/priority/priority_scheduler.cpp
        kernel/scheduler/control/control_scheduler.cpp
        kernel/scheduler/edf/edf_scheduler.cpp
        filesystem/file_access.cpp
        filesystem/file.cpp
        filesystem/stringpart.cpp
        filesystem/console/console_device.cpp
        filesystem/mountpointfs/mountpointfs.cpp
        filesystem/devfs/devfs.cpp
        filesystem/fat32/fat32.cpp
        filesystem/fat32/ff.cpp
        filesystem/fat32/diskio.cpp
        filesystem/fat32/wtoupper.cpp
        filesystem/fat32/ccsbcs.cpp
        stdlib_integration/libc_integration.cpp
        stdlib_integration/libstdcpp_integration.cpp
        e20/e20.cpp
        e20/unmember.cpp
        util/util.cpp
        util/unicode.cpp
        util/version.cpp
        util/crc16.cpp
        util/lcd44780.cpp
        )

## Add the architecture dependand sources to the list of files to build.
## ARCH_SRC will contain different source files depending on which
## architecture/board is selected in config/Makefile.inc

list(APPEND SOURCES ${ARCH_SRC})
list(APPEND SOURCES ${BOOT_FILE})

set(DFLAGS "-MMD -MP")

set(CMAKE_C_FLAGS "${DFLAGS} ${CFLAGS_BASE} -DCOMPILING_MIOSIX")
set(CMAKE_CXX_FLAGS "${DFLAGS} ${CXXFLAGS_BASE} -DCOMPILING_MIOSIX")
set(CMAKE_ASM_FLAGS ${AFLAGS_BASE})

set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> rcs <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> rcs <TARGET> <LINK_FLAGS> <OBJECTS>")

add_library(miosix STATIC ${SOURCES})

# TODO: run "perl _tools/kernel_global_objects.pl $(OBJ)"
