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

set(BOARDS
    stm32f407vg_stm32f4discovery
    stm32f429zi_stm32f4discovery
)

set(KPATH ${CMAKE_CURRENT_LIST_DIR})

set(KERNEL_SRC
    ${KPATH}/kernel/kernel.cpp
    ${KPATH}/kernel/sync.cpp
    ${KPATH}/kernel/error.cpp
    ${KPATH}/kernel/pthread.cpp
    ${KPATH}/kernel/stage_2_boot.cpp
    ${KPATH}/kernel/elf_program.cpp
    ${KPATH}/kernel/process.cpp
    ${KPATH}/kernel/process_pool.cpp
    ${KPATH}/kernel/timeconversion.cpp
    ${KPATH}/kernel/intrusive.cpp
    ${KPATH}/kernel/cpu_time_counter.cpp
    ${KPATH}/kernel/scheduler/priority/priority_scheduler.cpp
    ${KPATH}/kernel/scheduler/control/control_scheduler.cpp
    ${KPATH}/kernel/scheduler/edf/edf_scheduler.cpp
    ${KPATH}/filesystem/file_access.cpp
    ${KPATH}/filesystem/file.cpp
    ${KPATH}/filesystem/path.cpp
    ${KPATH}/filesystem/stringpart.cpp
    ${KPATH}/filesystem/pipe/pipe.cpp
    ${KPATH}/filesystem/console/console_device.cpp
    ${KPATH}/filesystem/mountpointfs/mountpointfs.cpp
    ${KPATH}/filesystem/devfs/devfs.cpp
    ${KPATH}/filesystem/fat32/fat32.cpp
    ${KPATH}/filesystem/fat32/ff.cpp
    ${KPATH}/filesystem/fat32/diskio.cpp
    ${KPATH}/filesystem/fat32/wtoupper.cpp
    ${KPATH}/filesystem/fat32/ccsbcs.cpp
    ${KPATH}/filesystem/littlefs/lfs_miosix.cpp
    ${KPATH}/filesystem/littlefs/lfs.c
    ${KPATH}/filesystem/littlefs/lfs_util.c
    ${KPATH}/filesystem/romfs/romfs.cpp
    ${KPATH}/stdlib_integration/libc_integration.cpp
    ${KPATH}/stdlib_integration/libstdcpp_integration.cpp
    ${KPATH}/e20/e20.cpp
    ${KPATH}/e20/unmember.cpp
    ${KPATH}/util/util.cpp
    ${KPATH}/util/unicode.cpp
    ${KPATH}/util/version.cpp
    ${KPATH}/util/crc16.cpp
    ${KPATH}/util/lcd44780.cpp
)

# Include the architecture options
include(${KPATH}/arch/CMakeLists.txt)

# Verify that all the required variables have been defined, otherwise abort
set(VARIABLES
    MIOSIX_OPT_BOARD
    ARCH_INC
    BOARD_INC
    MIOSIX_BOARD_CONFIG_INC
    MIOSIX_LINKER_SCRIPT
    AFLAGS_BASE
    CFLAGS_BASE
    CXXFLAGS_BASE
    LFLAGS_BASE
    ARCH_SRC
)
foreach(VARIABLE ${VARIABLES})
    if(NOT DEFINED ${VARIABLE})
        message(FATAL_ERROR "Board support package must define ${VARIABLE}")
    endif()
endforeach()

# The user can set a custom path for miosix_settings.h
set(MIOSIX_SETTINGS_PATH ${KPATH}/default CACHE PATH "Include directory for miosix_settings.h")

# Define the miosix library with kernel and architecture sources
add_library(miosix STATIC ${KERNEL_SRC} ${ARCH_SRC})
target_include_directories(miosix PUBLIC
    ${KPATH}
    ${KPATH}/arch/common
    ${ARCH_INC}
    ${BOARD_INC}
    ${MIOSIX_BOARD_CONFIG_INC}
    ${MIOSIX_SETTINGS_PATH}
)

# Configure compiler flags
target_compile_options(miosix PUBLIC
    $<$<COMPILE_LANGUAGE:ASM>:${AFLAGS_BASE}>
    $<$<COMPILE_LANGUAGE:C>:${CFLAGS_BASE}>
    $<$<COMPILE_LANGUAGE:CXX>:${CXXFLAGS_BASE}>
)

# Configure program command
set_property(TARGET miosix PROPERTY PROGRAM_CMDLINE ${PROGRAM_CMDLINE})

# Add COMPILING_MIOSIX define for private headers
target_compile_definitions(miosix PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:COMPILING_MIOSIX>)

# Configure linker file and options
set_property(TARGET miosix PROPERTY LINK_DEPENDS ${MIOSIX_LINKER_SCRIPT})
target_link_options(miosix PUBLIC ${LFLAGS_BASE})

add_custom_command(
    TARGET miosix PRE_LINK
    COMMAND perl ${KPATH}/_tools/kernel_global_objects.pl $<TARGET_OBJECTS:miosix>
    VERBATIM
    COMMAND_EXPAND_LISTS
)

option(MIOSIX_PRINT_BOARD_LIST "Prints a list of the available boards" OFF)
if(MIOSIX_PRINT_BOARD_LIST)
    message(STATUS "Available boards:")
    foreach(MIOSIX_OPT_BOARD ${BOARDS})
        message(STATUS "\t${MIOSIX_OPT_BOARD}")
    endforeach()
endif()
