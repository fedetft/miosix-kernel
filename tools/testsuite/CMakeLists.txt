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

cmake_minimum_required(VERSION 3.21)

set(MIOSIX_KPATH ${CMAKE_SOURCE_DIR}/../../miosix)
set(CMAKE_TOOLCHAIN_FILE ${MIOSIX_KPATH}/cmake/Toolchains/gcc.cmake)
set(CMAKE_BUILD_TYPE "Release")

project(testsuite C CXX ASM)

set(MIOSIX_BOARD stm32f407vg_stm32f4discovery)
set(MIOSIX_LINKER_SCRIPT processes.ld)

add_subdirectory(${MIOSIX_KPATH} miosix EXCLUDE_FROM_ALL)

# Kernel level program
add_executable(testsuite testsuite.cpp)
target_compile_definitions(miosix PUBLIC WITH_FILESYSTEM)
target_compile_definitions(miosix PUBLIC WITH_PROCESSES)
miosix_link_target(testsuite)

# Processes
miosix_add_process(test_process test_process/main.cpp)
miosix_add_process(test_execve test_execve/main.cpp)
miosix_add_process(test_global_dtor_ctor test_global_dtor_ctor/main.cpp)

# RomFS image
miosix_add_romfs_image(image
    PROGRAM_DEFAULT
    DIR_NAME bin
    KERNEL testsuite
    PROCESSES test_process test_execve test_global_dtor_ctor
)
