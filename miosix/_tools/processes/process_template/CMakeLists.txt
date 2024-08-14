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

project(Process C CXX ASM)

add_subdirectory(../../../libsyscalls libsyscalls EXCLUDE_FROM_ALL)

add_executable(hello main.cpp)
target_link_libraries(hello PRIVATE syscalls)

# Link libraries
target_link_libraries(hello PRIVATE
    -Wl,--start-group syscalls stdc++ c m gcc atomic -Wl,--end-group
)

# Post build commands
add_custom_command(
    TARGET hello POST_BUILD
    COMMAND arm-miosix-eabi-strip $<TARGET_FILE:hello>
    COMMENT "Stripped hello.elf"
)
add_custom_command(
    TARGET hello POST_BUILD
    COMMAND mx-postlinker $<TARGET_FILE:hello> --ramsize=16384 --stacksize=2048 --strip-sectheader
    COMMENT "Runned mx-postlinker on hello.elf"
)
add_custom_command(
    TARGET hello POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:hello> ${CMAKE_BINARY_DIR}/bin/$<TARGET_FILE_BASE_NAME:hello>
    COMMENT "Moved hello.elf into bin directory"
)
