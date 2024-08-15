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

# Create a target that builds the buildromfs tool
ExternalProject_Add(buildromfs
    SOURCE_DIR ${MIOSIX_KPATH}/_tools/filesystems
    INSTALL_COMMAND "" # Skip install
)

function(miosix_add_romfs_image)
    # Parse arguments after the target name
    cmake_parse_arguments(ROMFS "" "IMAGE_NAME;KERNEL;DIR_NAME" "PROCESSES" ${ARGN})

    # If the user did not provide a directory name, use "bin" as default
    if(NOT ROMFS_DIR_NAME)
        set(ROMFS_DIR_NAME bin)
        message("You did not provide a directory name for romfs, using ${ROMFS_DIR_NAME} as default")
    endif()

    # Move all processes binaries to a single directory
    set(ROMFS_PROCESSES_FILES "")
    foreach(ROMFS_PROCESS ${ROMFS_PROCESSES})
        add_custom_command(
            OUTPUT ${PROJECT_BINARY_DIR}/${ROMFS_DIR_NAME}/${ROMFS_PROCESS}
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${ROMFS_PROCESS}> ${PROJECT_BINARY_DIR}/${ROMFS_DIR_NAME}/${ROMFS_PROCESS}
            COMMENT "Moving $<TARGET_FILE_BASE_NAME:${ROMFS_PROCESS}> into ${ROMFS_DIR_NAME} directory"
        )
        list(APPEND ROMFS_PROCESSES_FILES ${PROJECT_BINARY_DIR}/${ROMFS_DIR_NAME}/${ROMFS_PROCESS})
    endforeach()

    # Create the romfs image with the given processes
    add_custom_command(
        OUTPUT ${ROMFS_IMAGE_NAME}-romfs.bin
        DEPENDS ${ROMFS_PROCESSES_FILES} buildromfs
        COMMAND ${MIOSIX_KPATH}/_tools/filesystems/buildromfs ${ROMFS_IMAGE_NAME}-romfs.bin --from-directory ${ROMFS_DIR_NAME}
        COMMENT "Building ${ROMFS_IMAGE_NAME}-romfs.bin"
    )

    # Create the kernel image from the elf
    add_custom_command(
        OUTPUT ${ROMFS_KERNEL}.bin
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${ROMFS_KERNEL}> ${ROMFS_KERNEL}.bin
        COMMENT "Creating ${ROMFS_KERNEL}.bin"
        VERBATIM
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
