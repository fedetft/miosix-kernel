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

include(MiosixPrintHelpers)

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

# Directory with header files for this board
set(MIOSIX_BOARD_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/board/${MIOSIX_OPT_BOARD})

# Include the board specific CMakeLists
include(${MIOSIX_BOARD_INC}/CMakeLists.txt)

option(MIOSIX_PRINT_LINKER_SCRIPT_LIST "Prints a list of the available linker scripts" OFF)
if(MIOSIX_PRINT_LINKER_SCRIPT_LIST)
    miosix_print_list(STATUS "Available linker scripts:" "${MIOSIX_LINKER_SCRIPT_LIST}")
endif()

# Normalize board options
set(MIOSIX_LINKER_SCRIPT "${MIOSIX_DEFAULT_LINKER_SCRIPT}" CACHE STRING "Linker script")
set_property(CACHE MIOSIX_LINKER_SCRIPT PROPERTY STRINGS ${MIOSIX_LINKER_SCRIPT_LIST})
if(NOT MIOSIX_LINKER_SCRIPT IN_LIST MIOSIX_LINKER_SCRIPT_LIST)
    # If there is no cached value, or the cached value is not in the list, complain
    miosix_expected_item_in_list("Invalid linker script '${MIOSIX_LINKER_SCRIPT}' specified with MIOSIX_LINKER_SCRIPT" "${MIOSIX_LINKER_SCRIPT_LIST}")
endif()

set(MIOSIX_BOARD_VARIANT "${MIOSIX_DEFAULT_BOARD_VARIANT}" CACHE STRING "Board variant")
set_property(CACHE MIOSIX_BOARD_VARIANT PROPERTY STRINGS ${MIOSIX_BOARD_VARIANT_LIST})
# Not all boards have variants
if(DEFINED MIOSIX_BOARD_VARIANT_LIST)
    if(NOT MIOSIX_BOARD_VARIANT IN_LIST MIOSIX_BOARD_VARIANT_LIST)
        # If there is no cached value, or the cached value is not in the list, set a default value
        miosix_expected_item_in_list("Invalid variant '${MIOSIX_BOARD_VARIANT}' specified with MIOSIX_BOARD_VARIANT" "${MIOSIX_BOARD_VARIANT_LIST}")
    else()
        # Add variant to board defines
        list(APPEND MIOSIX_BOARD_C_FLAGS "-D${MIOSIX_BOARD_VARIANT}")
        list(APPEND MIOSIX_BOARD_CXX_FLAGS "-D${MIOSIX_BOARD_VARIANT}")
    endif()
else()
    if(NOT MIOSIX_BOARD_VARIANT STREQUAL "")
        message(WARNING "Board '${MIOSIX_OPT_BOARD}' does not have variants but MIOSIX_BOARD_VARIANT is set to '${MIOSIX_BOARD_VARIANT}'")
    endif()
endif()

# Include the chip specific CMakeLists
include(${MIOSIX_CHIP_INC}/CMakeLists.txt)
