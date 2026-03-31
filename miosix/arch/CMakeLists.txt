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

include(ExpectedItemInList)
include(PrintList)

set(MIOSIX_BOARDS
    rp2040_raspberry_pi_pico
    stm32f407vg_stm32f4discovery
    stm32f429zi_stm32f4discovery
    stm32f767zi_nucleo
)

option(MIOSIX_PRINT_BOARD_LIST "Prints a list of the available boards" OFF)

# Target board option, this also implicitly select the target architecture
set(MIOSIX_OPT_BOARD NOT_SET CACHE STRING "Target board")
set_property(CACHE MIOSIX_OPT_BOARD PROPERTY STRINGS ${MIOSIX_BOARDS})
if(MIOSIX_OPT_BOARD STREQUAL NOT_SET)
    miosix_expected_item_in_list("You must specify a target board with MIOSIX_OPT_BOARD" "${MIOSIX_BOARDS}")
elseif(NOT MIOSIX_OPT_BOARD IN_LIST MIOSIX_BOARDS)
    miosix_expected_item_in_list("Selected board '${MIOSIX_OPT_BOARD}' is not in the MIOSIX_BOARDS list" "${MIOSIX_BOARDS}")
elseif(MIOSIX_PRINT_BOARD_LIST)
    # Optionally print the board list
    miosix_print_list(STATUS "Available boards:" "${MIOSIX_BOARDS}")
endif()

# Optimization flags are set with CMAKE_<LANG>_FLAGS_<CONFIG>
# The default values are in the toolchain file

# C++ Exception/rtti support disable flags.
# To save code size if not using C++ exceptions (nor some STL code which
# implicitly uses it) uncomment this option.
# -D__NO_EXCEPTIONS is used by Miosix to know if exceptions are used.
option(MIOSIX_DISABLE_EXCEPTIONS "Disables C++ exceptions/rtti support" OFF)
if(MIOSIX_DISABLE_EXCEPTIONS)
    set(MIOSIX_OPT_EXCEPT -fno-exceptions -fno-rtti -D__NO_EXCEPTIONS)
endif()

# Initialize C/C++ flags
set(MIOSIX_C_FLAGS
    -D_MIOSIX_BOARDNAME="${MIOSIX_OPT_BOARD}" -D_DEFAULT_SOURCE=1
    -ffunction-sections -Wall -Werror=return-type -g
)
set(MIOSIX_CXX_FLAGS
    -D_MIOSIX_BOARDNAME="${MIOSIX_OPT_BOARD}" -D_DEFAULT_SOURCE=1
    -ffunction-sections -Wall -Werror=return-type -g
)

# Directory with header files for this board
set(MIOSIX_BOARD_INC ${MIOSIX_KPATH}/arch/board/${MIOSIX_OPT_BOARD})

# Include the board and chip specific CMakeLists
include(${MIOSIX_BOARD_INC}/CMakeLists.txt)
include(${MIOSIX_ARCH_INC}/CMakeLists.txt)

# The user can set a custom path for board_settings.h
set(MIOSIX_CUSTOM_BOARD_SETTINGS_INC "" CACHE PATH "Include directory for custom board_settings.h")
mark_as_advanced(MIOSIX_CUSTOM_BOARD_SETTINGS_INC)
if(MIOSIX_CUSTOM_BOARD_SETTINGS_INC)
    set(MIOSIX_BOARD_SETTINGS_INC ${MIOSIX_CUSTOM_BOARD_SETTINGS_INC})
    message(NOTICE "You have set a custom path for board_settings.h")
else()
    set(MIOSIX_BOARD_SETTINGS_INC ${MIOSIX_CONFIG_PATH}/board/${MIOSIX_OPT_BOARD})
endif()

set(MIOSIX_LINKER_SCRIPT ${MIOSIX_DEFAULT_LINKER_SCRIPT} CACHE STRING "Linker script")
set_property(CACHE MIOSIX_LINKER_SCRIPT PROPERTY STRINGS ${MIOSIX_LINKER_SCRIPT_LIST})
if(NOT MIOSIX_LINKER_SCRIPT IN_LIST MIOSIX_LINKER_SCRIPT_LIST)
    # If there is no cached value, or the cached value is not in the list, complain
    miosix_expected_item_in_list("Invalid linker script '${MIOSIX_LINKER_SCRIPT}' specified with MIOSIX_LINKER_SCRIPT" "${MIOSIX_LINKER_SCRIPT_LIST}")
endif()

set(MIOSIX_BOARD_VARIANT ${MIOSIX_DEFAULT_BOARD_VARIANT} CACHE STRING "Board variant")
set_property(CACHE MIOSIX_BOARD_VARIANT PROPERTY STRINGS ${MIOSIX_BOARD_VARIANT_LIST})
# Not all boards have variants
if(DEFINED MIOSIX_BOARD_VARIANT_LIST)
    if(NOT MIOSIX_BOARD_VARIANT IN_LIST MIOSIX_BOARD_VARIANT_LIST)
        # If there is no cached value, or the cached value is not in the list, set a default value
        miosix_expected_item_in_list("Invalid variant '${MIOSIX_BOARD_VARIANT}' specified with MIOSIX_BOARD_VARIANT" "${MIOSIX_BOARD_VARIANT_LIST}")
    endif()
else()
    if(NOT MIOSIX_BOARD_VARIANT STREQUAL "")
        message(WARNING "Board '${MIOSIX_OPT_BOARD}' does not have variants but MIOSIX_BOARD_VARIANT is set to '${MIOSIX_BOARD_VARIANT}'")
    endif()
endif()
