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

# Remember to delete cmake cache for changes in CMakeLists.txt to have effect (or use --fresh to ignore it)

# Set here the path to miosix kernel
set(MIOSIX_KPATH ${CMAKE_SOURCE_DIR}/miosix CACHE PATH "Path to the miosix kernel")

# Set here your toolchain file
set(CMAKE_TOOLCHAIN_FILE ${MIOSIX_KPATH}/cmake/Toolchains/gcc.cmake CACHE PATH "Path to your toolchain file")
# set(CMAKE_TOOLCHAIN_FILE ${MIOSIX_KPATH}/cmake/Toolchains/clang.cmake CACHE PATH "Path to your toolchain file")

# Set your cmake build type here
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo")


# The project command is mandatory and needs to be added after setting CMAKE_TOOLCHAIN_FILE
project(Main C CXX ASM)

# Set here your config path for out-of-tree projects
# set(MIOSIX_CONFIG_PATH ${CMAKE_SOURCE_DIR}/miosix/config)

# Set here your board of choice
# set(MIOSIX_OPT_BOARD stm32f...)

add_subdirectory(${MIOSIX_KPATH} EXCLUDE_FROM_ALL)

# List here your source files (.s, .c and .cpp)
add_executable(main main.cpp)

# List here additional include directories
# target_include_directories(main PRIVATE here_your_includes)

# List here additional compile options
# target_compile_options(main PRIVATE here_your_compile_options)

# List here additional link directories
# target_link_directories(main PRIVATE here_your_link_directories)

# List here additional static libraries
# target_link_libraries(main PRIVATE here_your_libraries)

miosix_link_target(main)
