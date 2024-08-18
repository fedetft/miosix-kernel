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

# Copies processes binaries into a single directory
#
#   miosix_create_processes_dir(
#     DIR_NAME <dir_name>
#     PROCESSES <process1> <process2> ...
#   )
#
# This function addresses two use cases:
# - When you need to build a romfs image, you need all processes into a single directory
# - If you want to load processes on to an SD card for example, is useful to have all processes grouped togheter
function(miosix_create_processes_dir)
    cmake_parse_arguments(PROCS "" "DIR_NAME" "PROCESSES" ${ARGN})

    # Copy all processes binaries to a single directory
    foreach(ROMFS_PROCESS ${ROMFS_PROCESSES})
        add_custom_command(
            OUTPUT ${PROJECT_BINARY_DIR}/${ROMFS_DIR_NAME}/${ROMFS_PROCESS}
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${ROMFS_PROCESS}> ${PROJECT_BINARY_DIR}/${ROMFS_DIR_NAME}/${ROMFS_PROCESS}
            COMMENT "Copying process $<TARGET_FILE_BASE_NAME:${ROMFS_PROCESS}> into ${ROMFS_DIR_NAME} directory"
        )
        list(APPEND MIOSIX_${PROCS_DIR_NAME}_FILES ${PROJECT_BINARY_DIR}/${ROMFS_DIR_NAME}/${ROMFS_PROCESS})
    endforeach()

    # Move the list variable in the parent scope
    set(MIOSIX_${PROCS_DIR_NAME}_FILES ${MIOSIX_${PROCS_DIR_NAME}_FILES} PARENT_SCOPE)
endfunction()
