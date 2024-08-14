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

project(LibSyscalls C CXX ASM)

# syscalls is the library with userspace syscalls for processes

# Include the architecture options, we need the variables listed below
include(${MIOSIX_KPATH}/arch/CMakeLists.txt)

# Verify that all the required variables have been defined, otherwise abort
set(REQUIRED_VARIABLES
    MIOSIX_ARCH_INC
    MIOSIX_BOARD_INC
    MIOSIX_BOARD_SETTINGS_INC
    MIOSIX_CPU_FLAGS
)
foreach(VARIABLE ${REQUIRED_VARIABLES})
    if(NOT DEFINED ${VARIABLE})
        message(FATAL_ERROR "arch/CMakeLists.txt must define ${VARIABLE}")
    endif()
endforeach()

# Define the syscalls library
add_library(syscalls STATIC crt0.s crt1.cpp memoryprofiling.cpp)

target_include_directories(syscalls PUBLIC
    ${MIOSIX_KPATH}
    ${MIOSIX_KPATH}/arch/common
    ${MIOSIX_ARCH_INC}
    ${MIOSIX_BOARD_INC}
    ${MIOSIX_BOARD_SETTINGS_INC}
    ${MIOSIX_SETTINGS_PATH}
)

# ASM/C/C++ flags for processes
# The important flags are the CPU ones, -fpie and -msingle-pic-base
set(MIOSIX_PROCESSES_A_FLAGS ${MIOSIX_CPU_FLAGS})
set(MIOSIX_PROCESSES_C_FLAGS
    ${MIOSIX_CPU_FLAGS} -fpie -msingle-pic-base -ffunction-sections -Wall
    -Werror=return-type -D_DEFAULT_SOURCE=1 -c
)
set(MIOSIX_PROCESSES_CXX_FLAGS -std=c++14 ${MIOSIX_OPT_EXCEPT} ${MIOSIX_PROCESSES_C_FLAGS})
target_compile_options(syscalls PUBLIC
    $<$<COMPILE_LANGUAGE:ASM>:${MIOSIX_PROCESSES_A_FLAGS}>
    $<$<COMPILE_LANGUAGE:C>:${MIOSIX_PROCESSES_C_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${MIOSIX_PROCESSES_CXX_FLAGS}>
)

# Configure linker file and options needed to link agains this library
set(MIOSIX_PROCESSES_L_FLAGS
    ${MIOSIX_CPU_FLAGS} -fpie -msingle-pic-base -nostdlib -Wl,--gc-sections
    -Wl,-Map,process.map,-T${MIOSIX_KPATH}/libsyscalls/process.ld # TODO: Check process.map
    -Wl,-n,-pie,--spare-dynamic-tags,3,--target2=mx-data-rel
)
set_property(TARGET syscalls PROPERTY LINK_DEPENDS ${MIOSIX_KPATH}/libsyscalls/process.ld)
target_link_options(syscalls INTERFACE ${MIOSIX_PROCESSES_L_FLAGS})
