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

# Creates a custom target to program the board
function(miosix_add_program_target TARGET)
    get_target_property(PROGRAM_CMDLINE Miosix::interface-${OPT_BOARD} PROGRAM_CMDLINE)
    if(PROGRAM_CMDLINE STREQUAL "PROGRAM_CMDLINE-NOTFOUND")
        set(PROGRAM_CMDLINE st-flash --reset write <binary> 0x8000000)
    endif()

    list(TRANSFORM PROGRAM_CMDLINE REPLACE <binary> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.bin)
    list(TRANSFORM PROGRAM_CMDLINE REPLACE <hex> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.hex)

    add_custom_target(program-${TARGET} ${PROGRAM_CMDLINE}
        DEPENDS ${TARGET}
        VERBATIM
    )
endfunction()

# Function to link the Miosix libraries to a target and register the build command
function(miosix_link_target TARGET OPT_BOARD)
    if(NOT DEFINED OPT_BOARD)
        message(FATAL_ERROR "No board selected")
    endif()

    # Linker script and linking options are eredited from miosix libraries

    # Link libraries
    target_link_libraries(${TARGET} PRIVATE
        $<TARGET_OBJECTS:Miosix::boot-${OPT_BOARD}>
        $<LINK_GROUP:RESCAN,Miosix::miosix-${OPT_BOARD},stdc++,c,m,gcc,atomic>
    )

    # Add a post build command to create the hex file to flash on the board
    add_custom_command(
        TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${TARGET} ${TARGET}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary ${TARGET} ${TARGET}.bin
        BYPRODUCTS ${TARGET}.hex ${TARGET}.bin
        VERBATIM
    )

    # Generate custom build command to flash the target
    miosix_add_program_target(${TARGET})
endfunction()
