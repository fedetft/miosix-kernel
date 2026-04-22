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

# Version 3.21 allows overriding CACHE variables with normal variables
cmake_minimum_required(VERSION 3.21)

# Include Miosix's modules
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AddProcess)
include(AddProgramTarget)
include(AddRomfsImage)
include(CreateProcessesDir)
include(LinkTarget)

project(Miosix
    VERSION 3.0
    DESCRIPTION "OS kernel designed to run on 32bit microcontrollers."
    HOMEPAGE_URL "https://miosix.org"
    LANGUAGES C CXX
)
set(CMAKE_CXX_STANDARD 23 CACHE STRING "Project C++ standard")

# The user can set custom paths for miosix_settings.h and board_settings.h
set(MIOSIX_USER_CONFIG_PATH
    "${CMAKE_SOURCE_DIR}"
    CACHE PATH "Path of the Miosix configuration directory")
set(MIOSIX_USER_BOARD_SETTINGS_PATH
    "${MIOSIX_USER_CONFIG_PATH}/board/${MIOSIX_OPT_BOARD}"
    CACHE PATH "Include directory for board_settings.h")

# Default warning flags used for Miosix and projects that link with it.
# Configurable but not recommended.
set(MIOSIX_ASM_FLAGS "" CACHE PATH "Configurable Assembly flags for Miosix")
set(MIOSIX_C_FLAGS
    -Wall
    -Werror=return-type
    CACHE PATH "Configurable C flags for Miosix"
)
set(MIOSIX_CXX_FLAGS
    -Wall
    -Werror=return-type
    CACHE PATH "Configurable C++ flags for Miosix"
)
set(MIOSIX_EXE_LINKER_FLAGS "" CACHE PATH "Configurable linker flags for Miosix")

# C++ Exception/rtti support disable flags.
# To save code size if not using C++ exceptions (nor some STL code which
# implicitly uses it) uncomment this option.
# -D__NO_EXCEPTIONS is used by Miosix to know if exceptions are used.
option(MIOSIX_DISABLE_EXCEPTIONS "Disables C++ exceptions/rtti support" OFF)
if(MIOSIX_DISABLE_EXCEPTIONS)
    set(MIOSIX_OPT_EXCEPT -fno-exceptions -fno-rtti -D__NO_EXCEPTIONS)
endif()

# Option to enable/disable linker garbage collection
option(MIOSIX_ENABLE_LINKER_GC "Enables linker garbage collection" ON)
if(MIOSIX_ENABLE_LINKER_GC)
    set(LINKER_GC_FLAGS -ffunction-sections -fdata-sections)
    set(LINKER_GC_EXE_LINKER_FLAGS -Wl,--gc-sections)
endif()

# Include the architecture options
include(${CMAKE_CURRENT_SOURCE_DIR}/arch/CMakeLists.txt)

# Verify that all the required variables have been defined, otherwise abort
set(REQUIRED_VARIABLES
    MIOSIX_OPT_BOARD
    MIOSIX_CHIP_INC
    MIOSIX_BOARD_INC
    MIOSIX_LINKER_SCRIPT
    MIOSIX_BOARD_C_FLAGS
    MIOSIX_BOARD_CXX_FLAGS
    MIOSIX_CHIP_C_FLAGS
    MIOSIX_CHIP_CXX_FLAGS
    MIOSIX_BOARD_SRC
    MIOSIX_CHIP_SRC
)
foreach(VARIABLE ${REQUIRED_VARIABLES})
    if(NOT DEFINED ${VARIABLE})
        message(FATAL_ERROR "arch/CMakeLists.txt must define ${VARIABLE}")
    endif()
endforeach()

# Print build configuration
message(STATUS
        "Build Configuration\n"
        "   - CMAKE_CURRENT_SOURCE_DIR:         ${CMAKE_CURRENT_SOURCE_DIR}\n"
        "   - MIOSIX_USER_CONFIG_PATH:          ${MIOSIX_USER_CONFIG_PATH}\n"
        "   - MIOSIX_USER_BOARD_SETTINGS_PATH:  ${MIOSIX_USER_BOARD_SETTINGS_PATH}\n"
        "   - MIOSIX_GCC_PATH:                  ${MIOSIX_GCC_PATH}\n"
        "   - CMAKE_TOOLCHAIN_FILE:             ${CMAKE_TOOLCHAIN_FILE}\n"
        "   - MIOSIX_OPT_BOARD:                 ${MIOSIX_OPT_BOARD}\n"
        "   - MIOSIX_PRINT_BOARD_LIST:          ${MIOSIX_PRINT_BOARD_LIST}\n"
        "   - MIOSIX_PRINT_LINKER_SCRIPT_LIST:  ${MIOSIX_PRINT_LINKER_SCRIPT_LIST}\n"
        "   - CMAKE_CXX_STANDARD:               ${CMAKE_CXX_STANDARD}\n"
        "   - CMAKE_BUILD_TYPE:                 ${CMAKE_BUILD_TYPE}\n"
        "   Remember to delete cmake cache for changes in CMakeLists.txt to have effect (or use --fresh to ignore it)")

set(MIOSIX_KERNEL_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/thread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/lock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/sync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/pthread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/boot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/elf_program.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/process.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/process_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/timeconversion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/intrusive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/cpu_time_counter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/scheduler/priority/priority_scheduler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/scheduler/control/control_scheduler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/scheduler/edf/edf_scheduler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/file_access.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/file.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/path.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/stringpart.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/pipe/pipe.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/console/console_device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/mountpointfs/mountpointfs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/devfs/devfs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/exfat/exfat.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/fat32/fat32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/fatfs/ffunicode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/fatfs/diskio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/fatfs/ff.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/fatfs/wtoupper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/littlefs/lfs_miosix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/littlefs/lfs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/littlefs/lfs_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/romfs/romfs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kercalls/libc_integration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kercalls/libstdcpp_integration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/e20/e20.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/e20/unmember.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/util/util.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/util/unicode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/util/version.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/util/crc16.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/util/lcd44780.cpp
)

# Define the miosix library with kernel and architecture sources
add_library(miosix STATIC
    ${MIOSIX_KERNEL_SRC} 
    ${MIOSIX_BOARD_SRC}
    ${MIOSIX_CHIP_SRC}
)

target_include_directories(miosix PUBLIC
    ${MIOSIX_USER_CONFIG_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${MIOSIX_BOARD_SETTINGS_INC}
    ${CMAKE_CURRENT_SOURCE_DIR}/config/board/${MIOSIX_OPT_BOARD}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/arch
    ${MIOSIX_CPU_INC}
    ${MIOSIX_CHIP_INC}
    ${MIOSIX_BOARD_INC}
)

target_compile_options(miosix BEFORE PUBLIC
    -qkernelspace
    -D_MIOSIX_BOARDNAME="${MIOSIX_OPT_BOARD}"
    -D_DEFAULT_SOURCE=1
    $<$<COMPILE_LANGUAGE:ASM>:${MIOSIX_BOARD_ASM_FLAGS}>
    $<$<COMPILE_LANGUAGE:C>:${MIOSIX_BOARD_C_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${MIOSIX_BOARD_CXX_FLAGS}>
    $<$<COMPILE_LANGUAGE:ASM>:${MIOSIX_CHIP_ASM_FLAGS}>
    $<$<COMPILE_LANGUAGE:C>:${MIOSIX_CHIP_C_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${MIOSIX_CHIP_CXX_FLAGS}>
    ${MIOSIX_CPU_FLAGS}
    $<$<COMPILE_LANGUAGE:CXX>:${MIOSIX_OPT_EXCEPT}>
    ${LINKER_GC_FLAGS}
    $<$<COMPILE_LANGUAGE:ASM>:${MIOSIX_ASM_FLAGS}>
    $<$<COMPILE_LANGUAGE:C>:${MIOSIX_C_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${MIOSIX_CXX_FLAGS}>
)

# Configure program command as a property of the miosix library
set_property(TARGET miosix PROPERTY PROGRAM_CMDLINE ${PROGRAM_CMDLINE})

# Add COMPILING_MIOSIX define for private headers
target_compile_definitions(miosix PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:COMPILING_MIOSIX>)

# Configure linker file and options needed to link agains this library
set_property(TARGET miosix PROPERTY LINK_DEPENDS ${MIOSIX_BOARD_INC}/${MIOSIX_LINKER_SCRIPT})
target_link_options(miosix INTERFACE
    -qkernelspace
    ${MIOSIX_BOARD_EXE_LINKER_FLAGS}
    ${MIOSIX_CPU_FLAGS}
    ${MIOSIX_CHIP_EXE_LINKER_FLAGS}
    -Wl,-T${MIOSIX_BOARD_INC}/${MIOSIX_LINKER_SCRIPT}
    ${MIOSIX_OPT_EXCEPT}
    -nostdlib
    ${LINKER_GC_EXE_LINKER_FLAGS}
    ${MIOSIX_EXE_LINKER_FLAGS}
)

# Run the kernel_global_objects.pl script on all kernel objects
add_custom_command(
    TARGET miosix PRE_LINK
    COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/tools/kernel_global_objects.pl $<TARGET_OBJECTS:miosix> --prefix=${MIOSIX_PREFIX}
    VERBATIM
    COMMAND_EXPAND_LISTS
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Set multilib paths for Clang
    target_link_directories(miosix PUBLIC
        ${MIOSIX_GCC_PATH}/arm-miosix-eabi/lib/${MIOSIX_MULTILIB_PATH}
        ${MIOSIX_GCC_PATH}/lib/gcc/arm-miosix-eabi/15.2.0/${MIOSIX_MULTILIB_PATH}
    )
endif()
