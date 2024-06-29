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

# Parameters:
# - The path to the bps board_options.cmake file
# Required variables:
# - KPATH: Path to the kernel (.../miosix-kernel/miosix)
# Adds:
# - miosix-${OPT_BOARD}
function(miosix_add_board_library OPT_BOARD)
    if(NOT KPATH)
        message(FATAL_ERROR "KPATH must be defined to be the path to the miosix-kernel/miosix directory")
    endif()

    include(${KPATH}/arch/CMakeLists.txt)

    miosix_check_board_options_variables()

    miosix_add_miosix_library(${OPT_BOARD})
endfunction()

# Aborts if the required variables to generate the libraries are missing
function(miosix_check_board_options_variables)
    set(VARIABLES
        OPT_BOARD
        ARCH_INC
        BOARD_INC
        BOARD_CONFIG_INC
        LINKER_SCRIPT
        AFLAGS_BASE
        CFLAGS_BASE
        CXXFLAGS_BASE
        LFLAGS_BASE
        ARCH_SRC
    )
    foreach(VARIABLE ${VARIABLES})
        if(NOT DEFINED ${VARIABLE})
            message(FATAL_ERROR "Board support package must define ${VARIABLE}")
        endif()
    endforeach()
endfunction()

# Add a library with all the kernel source code called miosix-${OPT_BOARD}
function(miosix_add_miosix_library OPT_BOARD)
    set(MIOSIX_LIB miosix-${OPT_BOARD})
    add_library(${MIOSIX_LIB} STATIC ${KERNEL_SRC} ${ARCH_SRC})

    target_include_directories(${MIOSIX_LIB} PUBLIC
        ${KPATH}
        ${KPATH}/arch/common
        ${ARCH_INC}
        ${BOARD_INC}
        ${BOARD_CONFIG_INC}
    )

    # The user can set a custom path for miosix_settings.h
    if(DEFINED CUSTOM_MIOSIX_SETTINGS_PATH)
        target_include_directories(${MIOSIX_LIB} PUBLIC ${CUSTOM_MIOSIX_SETTINGS_PATH})
    else()
        # By default miosix/default/config/miosix_settings.h is used
        target_include_directories(${MIOSIX_LIB} PUBLIC ${KPATH}/default)
    endif()

    # Configure compiler flags
    target_compile_options(${MIOSIX_LIB} PUBLIC
        $<$<COMPILE_LANGUAGE:ASM>:${AFLAGS_BASE}>
        $<$<COMPILE_LANGUAGE:C>:${CFLAGS_BASE}>
        $<$<COMPILE_LANGUAGE:CXX>:${CXXFLAGS_BASE}>
    )

    # Configure program command
    set_property(TARGET ${MIOSIX_LIB} PROPERTY PROGRAM_CMDLINE ${PROGRAM_CMDLINE})

    # Add COMPILING_MIOSIX define for private headers
    target_compile_definitions(${MIOSIX_LIB} PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:COMPILING_MIOSIX>)

    # Configure linker file and options
    set_property(TARGET ${MIOSIX_LIB} PROPERTY LINK_DEPENDS ${LINKER_SCRIPT})
    target_link_options(${MIOSIX_LIB} PUBLIC ${LFLAGS_BASE})

    add_custom_command(
        TARGET ${MIOSIX_LIB} PRE_LINK
        COMMAND perl ${KPATH}/_tools/kernel_global_objects.pl $<TARGET_OBJECTS:${MIOSIX_LIB}>
        VERBATIM
        COMMAND_EXPAND_LISTS
    )
endfunction()
