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

project(ProcessesExamples C CXX ASM)

# Set Miosix definitions and options
add_compile_definitions($<$<COMPILE_LANGUAGE:C,CXX>:PARSING_FROM_IDE>)
add_compile_definitions($<$<COMPILE_LANGUAGE:C,CXX>:WITH_PROCESSES>)
set(MIOSIX_OPT_BOARD stm32f429zi_stm32f4discovery CACHE STRING "Target board")
set(MIOSIX_LINKER_SCRIPT stm32_2m+256k_rom_processes.ld CACHE FILEPATH "Linker script")

add_subdirectory(../.. miosix EXCLUDE_FROM_ALL)
add_subdirectory(../../libsyscalls libsyscalls EXCLUDE_FROM_ALL)

include(LinkTarget)
include(AddProcess)
include(RomFS)

# Kernel level program
add_executable(main main.cpp)
miosix_link_target(main)

# Processes
miosix_add_process(process1 process1.cpp)
miosix_add_process(process2 process2.cpp)

# RomFS image
miosix_add_romfs_image(
    IMAGE_NAME image
    DIR_NAME bin
    KERNEL main
    PROCESSES process1 process2
)
