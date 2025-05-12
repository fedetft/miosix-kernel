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

set(CMAKE_CXX_STANDARD 14 CACHE STRING "Project C++ standard")

# To avoid printing an empty string
if(MIOSIX_PRINT_BOARD_LIST)
    set(MIOSIX_PRINT_BOARD_LIST ON)
else()
    set(MIOSIX_PRINT_BOARD_LIST OFF)
endif()

# Print build configuration
message(STATUS
        "Build Configuration\n"
        "   - MIOSIX_KPATH:             ${MIOSIX_KPATH}\n"
        "   - MIOSIX_CONFIG_PATH:       ${MIOSIX_CONFIG_PATH}\n"
        "   - MIOSIX_GCC_PATH:          ${MIOSIX_GCC_PATH}\n"
        "   - CMAKE_TOOLCHAIN_FILE:     ${CMAKE_TOOLCHAIN_FILE}\n"
        "   - MIOSIX_OPT_BOARD:         ${MIOSIX_OPT_BOARD}\n"
        "   - MIOSIX_PRINT_BOARD_LIST:  ${MIOSIX_PRINT_BOARD_LIST}\n"
        "   - CMAKE_CXX_STANDARD:       ${CMAKE_CXX_STANDARD}\n"
        "   - CMAKE_BUILD_TYPE:         ${CMAKE_BUILD_TYPE}\n"
        "   Remember to delete cmake cache for changes in CMakeLists.txt to have effect (or use --fresh to ignore it)")

set(MIOSIX_KERNEL_SRC
    ${MIOSIX_KPATH}/kernel/thread.cpp
    ${MIOSIX_KPATH}/kernel/lock.cpp
    ${MIOSIX_KPATH}/kernel/sync.cpp
    ${MIOSIX_KPATH}/kernel/error.cpp
    ${MIOSIX_KPATH}/kernel/pthread.cpp
    ${MIOSIX_KPATH}/kernel/boot.cpp
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
    ${MIOSIX_KPATH}/kercalls/libc_integration.cpp
    ${MIOSIX_KPATH}/kercalls/libstdcpp_integration.cpp
    ${MIOSIX_KPATH}/e20/e20.cpp
    ${MIOSIX_KPATH}/e20/unmember.cpp
    ${MIOSIX_KPATH}/util/util.cpp
    ${MIOSIX_KPATH}/util/unicode.cpp
    ${MIOSIX_KPATH}/util/version.cpp
    ${MIOSIX_KPATH}/util/crc16.cpp
    ${MIOSIX_KPATH}/util/lcd44780.cpp
)

# The user can set a custom path for miosix_settings.h
set(MIOSIX_CONFIG_PATH ${MIOSIX_KPATH}/config CACHE PATH "Include directory for miosix config")
mark_as_advanced(MIOSIX_CONFIG_PATH)
if(NOT MIOSIX_CONFIG_PATH STREQUAL ${MIOSIX_KPATH}/config)
    message(NOTICE "You have set a custom path for miosix config: ${MIOSIX_CONFIG_PATH}")
endif()

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

# Define the miosix library with kernel and architecture sources
add_library(miosix STATIC ${MIOSIX_KERNEL_SRC} ${MIOSIX_ARCH_SRC})

target_include_directories(miosix PUBLIC
    ${MIOSIX_CONFIG_PATH}
    ${MIOSIX_KPATH}
    ${MIOSIX_KPATH}/arch/common
    ${MIOSIX_ARCH_INC}
    ${MIOSIX_BOARD_INC}
    ${MIOSIX_BOARD_SETTINGS_INC}
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
    COMMAND perl ${MIOSIX_KPATH}/_tools/kernel_global_objects.pl $<TARGET_OBJECTS:miosix> --prefix=${MIOSIX_PREFIX}
    VERBATIM
    COMMAND_EXPAND_LISTS
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Set multilib paths for Clang
    target_link_directories(miosix PUBLIC
        ${MIOSIX_GCC_PATH}/arm-miosix-eabi/lib/${MIOSIX_MULTILIB_PATH}
        ${MIOSIX_GCC_PATH}/lib/gcc/arm-miosix-eabi/9.2.0/${MIOSIX_MULTILIB_PATH}
    )
endif()
