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
# Creates three libraries:
# - Miosix::interface-${BOARD_NAME}
# - Miosix::boot-${BOARD_NAME}
# - Miosix::miosix-${BOARD_NAME}
function(miosix_add_board_libraries BOARD_OPTIONS_FILE)
    if(NOT KPATH)
        message(FATAL_ERROR "KPATH must be defined to be the path to the miosix-kernel/miosix directory")
    endif()

    include(${BOARD_OPTIONS_FILE})
    include(${KPATH}/cmake/kernel_sources.cmake)

    miosix_check_board_options_variables()

    miosix_add_interface_library(${BOARD_NAME})
    miosix_add_boot_library(${BOARD_NAME})
    miosix_add_miosix_library(${BOARD_NAME})
endfunction()

# Aborts if the required variables to generate the libraries are missing
function(miosix_check_board_options_variables)
    set(VARIABLES
        BOARD_NAME
        ARCH_PATH
        BOARD_PATH
        BOARD_CONFIG_PATH
        BOOT_FILE
        LINKER_SCRIPT
        AFLAGS
        LFLAGS
        CFLAGS
        CXXFLAGS
        ARCH_SRC
    )
    foreach(VARIABLE ${VARIABLES})
        if(NOT DEFINED ${VARIABLE})
            message(FATAL_ERROR "You must define ${VARIABLE} in your board_options.cmake file")
        endif()
    endforeach()
endfunction()

# Adds an interface library with all the include directories and compile options needed to compile kernel code 
function(miosix_add_interface_library BOARD_NAME)
    set(INTERFACE_LIB interface-${BOARD_NAME})
    add_library(${INTERFACE_LIB} INTERFACE EXCLUDE_FROM_ALL)
    add_library(Miosix::${INTERFACE_LIB} ALIAS ${INTERFACE_LIB})

    target_include_directories(${INTERFACE_LIB} INTERFACE
        ${KPATH}
        ${KPATH}/arch/common
        ${ARCH_PATH}
        ${BOARD_PATH}
        ${BOARD_CONFIG_PATH}
    )

    # The user can set a custom path for miosix_settings.h
    if(DEFINED CUSTOM_MIOSIX_SETTINGS_PATH)
        target_include_directories(${INTERFACE_LIB} INTERFACE ${CUSTOM_MIOSIX_SETTINGS_PATH})
    else()
        # By default miosix/default/config/miosix_settings.h is used
        target_include_directories(${INTERFACE_LIB} INTERFACE ${KPATH}/default)
    endif()

    # Configure compiler flags
    target_compile_options(${INTERFACE_LIB} INTERFACE
        $<$<COMPILE_LANGUAGE:ASM>:${AFLAGS}>
        $<$<COMPILE_LANGUAGE:C>:${DFLAGS} ${CFLAGS}>
        $<$<COMPILE_LANGUAGE:CXX>:${DFLAGS} ${CXXFLAGS}>
    )

    # Configure program command
    set_property(TARGET ${INTERFACE_LIB} PROPERTY PROGRAM_CMDLINE ${PROGRAM_CMDLINE})
endfunction()

# Add an object library with only the boot file called boot-${BOARD_NAME}
function(miosix_add_boot_library BOARD_NAME)
    set(BOOT_LIB boot-${BOARD_NAME})
    add_library(${BOOT_LIB} OBJECT ${BOOT_FILE})
    add_library(Miosix::${BOOT_LIB} ALIAS ${BOOT_LIB})
    target_link_libraries(${BOOT_LIB} PUBLIC Miosix::interface-${BOARD_NAME})

    # Add COMPILING_MIOSIX define for private headers
    target_compile_definitions(${BOOT_LIB} PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:COMPILING_MIOSIX>)
endfunction()

# Add a library with all the kernel source code called miosix-${BOARD_NAME}
function(miosix_add_miosix_library BOARD_NAME)
    set(MIOSIX_LIB miosix-${BOARD_NAME})
    add_library(${MIOSIX_LIB} STATIC ${KERNEL_SRC} ${ARCH_SRC})
    add_library(Miosix::${MIOSIX_LIB} ALIAS ${MIOSIX_LIB})
    target_link_libraries(${MIOSIX_LIB} PUBLIC Miosix::interface-${BOARD_NAME})

    # Add COMPILING_MIOSIX define for private headers
    target_compile_definitions(${MIOSIX_LIB} PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:COMPILING_MIOSIX>)

    # Configure linker file and options
    set_property(TARGET ${INTERFACE_LIB} PROPERTY LINK_DEPENDS ${LINKER_SCRIPT})
    target_link_options(${MIOSIX_LIB} PUBLIC ${LFLAGS})

    add_custom_command(
        TARGET ${MIOSIX_LIB} PRE_LINK
        COMMAND perl ${KPATH}/_tools/kernel_global_objects.pl $<TARGET_OBJECTS:${MIOSIX_LIB}>
        VERBATIM
        COMMAND_EXPAND_LISTS
    )
endfunction()
