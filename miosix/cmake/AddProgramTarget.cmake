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
#
#   miosix_add_program_target(<target> [DEPENDS <target1> <target2> ...])
#
function(miosix_add_program_target TARGET)
    cmake_parse_arguments(PROGRAM "" "" "DEPENDS" ${ARGN})

    if(NOT PROGRAM_DEPENDS)
        set(PROGRAM_DEPENDS ${TARGET}_bin ${TARGET}_hex)
    endif()

    get_target_property(PROGRAM_CMDLINE miosix PROGRAM_CMDLINE)
    if(PROGRAM_CMDLINE STREQUAL "PROGRAM_CMDLINE-NOTFOUND")
        set(PROGRAM_CMDLINE st-flash --connect-under-reset --reset write <binary> 0x8000000)
    endif()

    list(TRANSFORM PROGRAM_CMDLINE REPLACE <binary> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.bin)
    list(TRANSFORM PROGRAM_CMDLINE REPLACE <hex> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.hex)

    add_custom_target(${TARGET}_program ${PROGRAM_CMDLINE}
        DEPENDS ${PROGRAM_DEPENDS}
        VERBATIM
    )
endfunction()
