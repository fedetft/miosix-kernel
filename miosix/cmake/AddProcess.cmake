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
#   miosix_add_process(<target> <source1> <source2> ...)
#
# What is does:
# - Create an executable target with the given sources
# - Link the libraries required by processes
# - Tell the linker to produce a map file
# - Run strip and mx-postlinker on the executable
function(miosix_add_process TARGET SOURCES)
    # Define the executable with its sources
    add_executable(${TARGET} ${SOURCES})

    # Link syscalls and other libraries
    target_link_libraries(${TARGET} PRIVATE
        -Wl,--start-group syscalls stdc++ c m gcc atomic -Wl,--end-group
    )

    # Tell the linker to produce the map file
    target_link_options(${TARGET} PRIVATE -Wl,-Map,$<TARGET_FILE_DIR:${TARGET}>/$<TARGET_FILE_BASE_NAME:${TARGET}>.map)

    # Strin unnecessary sections from the ELF
    add_custom_command(
        TARGET ${TARGET} POST_BUILD
        COMMAND arm-miosix-eabi-strip $<TARGET_FILE:${TARGET}>
        COMMENT "Stripping $<TARGET_FILE_NAME:${TARGET}>"
    )

    # Run mx-postlinker to strip the section header, string table and
    # setting Miosix specic options in the dynamic segnment
    add_custom_command(
        TARGET ${TARGET} POST_BUILD
        COMMAND mx-postlinker $<TARGET_FILE:${TARGET}> --ramsize=16384 --stacksize=2048 --strip-sectheader
        # BYPRODUCTS $<TARGET_FILE:${TARGET}>
        COMMENT "Running mx-postlinker on $<TARGET_FILE_NAME:${TARGET}>"
    )
endfunction()
