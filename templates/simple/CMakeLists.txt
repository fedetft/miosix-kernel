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

# Set the path to the Miosix kernel
set(MIOSIX_KPATH ${CMAKE_SOURCE_DIR}/../../miosix)
# Configure CMake toolchain and build type.
# This must to be done before the project() command
set(CMAKE_TOOLCHAIN_FILE ${MIOSIX_KPATH}/cmake/Toolchains/gcc.cmake)
# set(CMAKE_TOOLCHAIN_FILE ${MIOSIX_KPATH}/cmake/Toolchains/clang.cmake)
set(CMAKE_BUILD_TYPE "Release")

project(Main C CXX ASM)

# Select the board (mandatory). See the options in miosix/arch/CMakeLists.txt
set(MIOSIX_BOARD stm32...)
# User config path and linker script selection are optional
# set(MIOSIX_USER_CONFIG_PATH ${CMAKE_SOURCE_DIR}/config)
# set(MIOSIX_LINKER_SCRIPT unikernel.ld)

# Include the Miosix kernel's project
add_subdirectory(${MIOSIX_KPATH} miosix EXCLUDE_FROM_ALL)

# Define a kernel level program
add_executable(main main.cpp)
# target_include_directories(main PRIVATE here_your_includes)
# target_compile_options(main PRIVATE here_your_compile_options)
# target_link_directories(main PRIVATE here_your_link_directories)
# target_link_libraries(main PRIVATE here_your_libraries)

# Link the main target with Miosix. When specifying PROGRAM_DEFAULT, the
# `program' target will be available to flash it (type `make program' to use it)
miosix_link_target(main PROGRAM_DEFAULT)
