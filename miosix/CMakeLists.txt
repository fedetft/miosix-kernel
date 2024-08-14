# Copyright (C) 2024 by Skyward
#
# This program is free software; you can redistribute it and/or
# it under the terms of the GNU General Public License as published
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# As a special exception, if other files instantiate templates or use
# macros or inline functions from this file, or you compile this file
# and link it with other works to produce a work based on this file,
# this file does not by itself cause the resulting work to be covered
# by the GNU General Public License. However the source code for this
# file must still be made available in accordance with the GNU
# Public License. This exception does not invalidate any other
# why a work based on this file might be covered by the GNU General
# Public License.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>

cmake_minimum_required(VERSION 3.16)

project(Miosix
    VERSION 2.7
    DESCRIPTION "OS kernel designed to run on 32bit microcontrollers."
    HOMEPAGE_URL "https://miosix.org"
    LANGUAGES C CXX
)

set(MIOSIX_KPATH ${CMAKE_CURRENT_LIST_DIR} CACHE PATH "Path to the miosix kernel")

set(MIOSIX_KERNEL_SRC
    ${MIOSIX_KPATH}/kernel/kernel.cpp
    ${MIOSIX_KPATH}/kernel/sync.cpp
    ${MIOSIX_KPATH}/kernel/error.cpp
    ${MIOSIX_KPATH}/kernel/pthread.cpp
    ${MIOSIX_KPATH}/kernel/stage_2_boot.cpp
    ${MIOSIX_KPATH}/kernel/elf_program.cpp
    ${MIOSIX_KPATH}/kernel/process.cpp
    ${MIOSIX_KPATH}/kernel/process_pool.cpp
    ${MIOSIX_KPATH}/kernel/timeconversion.cpp
    ${MIOSIX_KPATH}/kernel/intrusive.cpp
    ${MIOSIX_KPATH}/kernel/cpu_time_counter.cpp
    ${MIOSIX_KPATH}/kernel/scheduler/priority/priority_scheduler.cpp
    ${MIOSIX_KPATH}/kernel/scheduler/control/control_scheduler.cpp
    ${MIOSIX_KPATH}/kernel/scheduler/edf/edf_scheduler.cpp
    ${MIOSIX_KPATH}/filesystem/file_access.cpp
    ${MIOSIX_KPATH}/filesystem/file.cpp
    ${MIOSIX_KPATH}/filesystem/path.cpp
    ${MIOSIX_KPATH}/filesystem/stringpart.cpp
    ${MIOSIX_KPATH}/filesystem/pipe/pipe.cpp
    ${MIOSIX_KPATH}/filesystem/console/console_device.cpp
    ${MIOSIX_KPATH}/filesystem/mountpointfs/mountpointfs.cpp
    ${MIOSIX_KPATH}/filesystem/devfs/devfs.cpp
    ${MIOSIX_KPATH}/filesystem/fat32/fat32.cpp
    ${MIOSIX_KPATH}/filesystem/fat32/ff.cpp
    ${MIOSIX_KPATH}/filesystem/fat32/diskio.cpp
    ${MIOSIX_KPATH}/filesystem/fat32/wtoupper.cpp
    ${MIOSIX_KPATH}/filesystem/fat32/ccsbcs.cpp
    ${MIOSIX_KPATH}/filesystem/littlefs/lfs_miosix.cpp
    ${MIOSIX_KPATH}/filesystem/littlefs/lfs.c
    ${MIOSIX_KPATH}/filesystem/littlefs/lfs_util.c
    ${MIOSIX_KPATH}/filesystem/romfs/romfs.cpp
    ${MIOSIX_KPATH}/stdlib_integration/libc_integration.cpp
    ${MIOSIX_KPATH}/stdlib_integration/libstdcpp_integration.cpp
    ${MIOSIX_KPATH}/e20/e20.cpp
    ${MIOSIX_KPATH}/e20/unmember.cpp
    ${MIOSIX_KPATH}/util/util.cpp
    ${MIOSIX_KPATH}/util/unicode.cpp
    ${MIOSIX_KPATH}/util/version.cpp
    ${MIOSIX_KPATH}/util/crc16.cpp
    ${MIOSIX_KPATH}/util/lcd44780.cpp
)

# Include the architecture options
include(${MIOSIX_KPATH}/arch/CMakeLists.txt)

# Verify that all the required variables have been defined, otherwise abort
set(REQUIRED_VARIABLES
    MIOSIX_OPT_BOARD
    MIOSIX_ARCH_INC
    MIOSIX_BOARD_INC
    MIOSIX_BOARD_SETTINGS_INC
    MIOSIX_LINKER_SCRIPT
    MIOSIX_A_FLAGS
    MIOSIX_C_FLAGS
    MIOSIX_CXX_FLAGS
    MIOSIX_L_FLAGS
    MIOSIX_ARCH_SRC
)
foreach(VARIABLE ${REQUIRED_VARIABLES})
    if(NOT DEFINED ${VARIABLE})
        message(FATAL_ERROR "arch/CMakeLists.txt must define ${VARIABLE}")
    endif()
endforeach()

# The user can set a custom path for miosix_settings.h
set(MIOSIX_SETTINGS_PATH ${MIOSIX_KPATH}/default CACHE PATH "Include directory for miosix_settings.h")
mark_as_advanced(MIOSIX_SETTINGS_PATH)
if(NOT MIOSIX_SETTINGS_PATH STREQUAL ${MIOSIX_KPATH}/default)
    message(NOTICE "You have set a custom path for miosix_settings.h")
endif()

# Define the miosix library with kernel and architecture sources
add_library(miosix STATIC ${MIOSIX_KERNEL_SRC} ${MIOSIX_ARCH_SRC})

target_include_directories(miosix PUBLIC
    ${MIOSIX_KPATH}
    ${MIOSIX_KPATH}/arch/common
    ${MIOSIX_ARCH_INC}
    ${MIOSIX_BOARD_INC}
    ${MIOSIX_BOARD_SETTINGS_INC}
    ${MIOSIX_SETTINGS_PATH}
)

target_compile_options(miosix PUBLIC
    $<$<COMPILE_LANGUAGE:ASM>:${MIOSIX_A_FLAGS}>
    $<$<COMPILE_LANGUAGE:C>:${MIOSIX_C_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${MIOSIX_CXX_FLAGS}>
)

# Configure program command as a propery of the miosix library
set_property(TARGET miosix PROPERTY PROGRAM_CMDLINE ${PROGRAM_CMDLINE})

# Add COMPILING_MIOSIX define for private headers
target_compile_definitions(miosix PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:COMPILING_MIOSIX>)

# Configure linker file and options needed to link agains this library
set_property(TARGET miosix PROPERTY LINK_DEPENDS ${MIOSIX_BOARD_INC}/${MIOSIX_LINKER_SCRIPT})
target_link_options(miosix INTERFACE ${MIOSIX_L_FLAGS})

# Run the kernel_global_objects.pl script on all kernel objects
add_custom_command(
    TARGET miosix PRE_LINK
    COMMAND perl ${MIOSIX_KPATH}/_tools/kernel_global_objects.pl $<TARGET_OBJECTS:miosix>
    VERBATIM
    COMMAND_EXPAND_LISTS
)

# Optionally print the board list
option(MIOSIX_PRINT_BOARD_LIST "Prints a list of the available boards" OFF)
if(MIOSIX_PRINT_BOARD_LIST)
    message(STATUS "Available boards:")
    foreach(MIOSIX_OPT_BOARD ${MIOSIX_BOARDS})
        message(STATUS "\t${MIOSIX_OPT_BOARD}")
    endforeach()
endif()
