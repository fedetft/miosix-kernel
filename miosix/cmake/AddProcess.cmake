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

# Function to create a Miosix process
#
#   miosix_add_process(<target> <source1> <source2> ... [RAM_SIZE ram-size]
#                      [STACK_SIZE stack-size] [NO_STRIP_SECTHEADER])
#
# What is does:
# - Create an executable target with the given sources
# - Link the libraries required by processes
# - Tell the linker to produce a map file
# - Run strip and mx-postlinker on the executable
# You can specify ram and stack size reservations with the RAM_SIZE and
# STACK_SIZE parameters. Section headers are stripped by default, but it can
# be disabled by passing NO_STRIP_SECTHEADER.
function(miosix_add_process TARGET SOURCES)
    cmake_parse_arguments(PARSE_ARGV 0 PROC "NO_STRIP_SECTHEADER" "RAM_SIZE;STACK_SIZE" "")

    # Define the executable with its sources
    add_executable(${TARGET} ${SOURCES})

    target_compile_options(${TARGET} PUBLIC ${MIOSIX_CPU_FLAGS} -ffunction-sections)
    target_link_options(${TARGET} PUBLIC -Wl,--gc-sections)

    # Strip unnecessary sections from the ELF
    add_custom_command(
        TARGET ${TARGET} POST_BUILD
        COMMAND arm-miosix-eabi-strip $<TARGET_FILE:${TARGET}>
        COMMENT "Stripping $<TARGET_FILE_NAME:${TARGET}>"
    )

    # Run mx-postlinker to strip the section header, string table and
    # setting Miosix specific options in the dynamic segment
    if(NOT PROC_RAM_SIZE)
        set(PROC_RAM_SIZE 16384)
    endif()
    if(NOT PROC_STACK_SIZE)
        set(PROC_STACK_SIZE 2048)
    endif()
    if(NOT PROC_NO_STRIP_SECTHEADER)
        set(PROC_SECTHEADER --strip-sectheader)
    endif()
    add_custom_command(
        TARGET ${TARGET} POST_BUILD
        COMMAND 
            mx-postlinker $<TARGET_FILE:${TARGET}> 
            --ramsize=${PROC_RAM_SIZE} 
            --stacksize=${PROC_STACK_SIZE} 
            ${PROC_SECTHEADER}
        # BYPRODUCTS $<TARGET_FILE:${TARGET}>
        COMMENT "Running mx-postlinker on $<TARGET_FILE_NAME:${TARGET}>"
    )
endfunction()
