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

include(ExternalProject)
include(CreateProcessesDir)

# Create a target that builds the buildromfs tool
ExternalProject_Add(buildromfs
    SOURCE_DIR ${MIOSIX_KPATH}/_tools/filesystems
    INSTALL_COMMAND "" # Skip install
)

# Create a target that builds the romfs image and combines it the kernel into a single binary image
#
#   miosix_add_romfs_image(
#     IMAGE_NAME <name>
#     KERNEL <kernel>
#     DIR_NAME <dir_name>
#     PROCESSES <process1> <process2> ...
#   )
#
# What it does:
# - Copies all processes binaries to a single directory named <dir_name>
# - Creates a romfs image with of the directory <dir_name>
# - Combines the kernel and the romfs image into a single binary image
# - Registers a custom target (named <dir_name>) with to run the above steps
function(miosix_add_romfs_image)
    cmake_parse_arguments(ROMFS "" "IMAGE_NAME;KERNEL;DIR_NAME" "PROCESSES" ${ARGN})

    # If the user did not provide a directory name, use "bin" as default
    if(NOT ROMFS_DIR_NAME)
        set(ROMFS_DIR_NAME bin)
        message(NOTICE "You did not provide a directory name for romfs, using ${ROMFS_DIR_NAME} as default")
    endif()

    # Copy all processes binaries to a single directory
    miosix_create_processes_dir(
        DIR_NAME ${ROMFS_DIR_NAME}
        PROCESSES ${ROMFS_PROCESSES}
    )

    # Create the romfs image with the given processes
    add_custom_command(
        OUTPUT ${ROMFS_IMAGE_NAME}-romfs.bin
        DEPENDS ${MIOSIX_${ROMFS_DIR_NAME}_FILES} buildromfs
        COMMAND ${MIOSIX_KPATH}/_tools/filesystems/buildromfs ${ROMFS_IMAGE_NAME}-romfs.bin --from-directory ${ROMFS_DIR_NAME}
        COMMENT "Building ${ROMFS_IMAGE_NAME}-romfs.bin"
    )

    # Combin kernel and romfs images
    add_custom_command(
        OUTPUT ${ROMFS_IMAGE_NAME}.bin
        DEPENDS ${ROMFS_KERNEL}.bin ${ROMFS_IMAGE_NAME}-romfs.bin
        COMMAND perl ${MIOSIX_KPATH}/_tools/filesystems/mkimage.pl ${ROMFS_IMAGE_NAME}.bin ${ROMFS_KERNEL}.bin ${ROMFS_IMAGE_NAME}-romfs.bin
        COMMENT "Combining ${ROMFS_KERNEL}.bin and ${ROMFS_IMAGE_NAME}-romfs.bin into ${ROMFS_IMAGE_NAME}.bin"
    )

    # Create the custom romfs target
    add_custom_target(${ROMFS_IMAGE_NAME} ALL DEPENDS ${PROJECT_BINARY_DIR}/${ROMFS_IMAGE_NAME}.bin)
endfunction()
