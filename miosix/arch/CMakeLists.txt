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
    atsam4lc2aa_generic
    efm32g222f128_generic
    efm32gg332f1024_wandstem
    lpc2138_generic
    lpc2138_miosix_board
    nrf52840_generic
    rp2040_raspberry_pi_pico
    stm32f030r8_stm32f0discovery
    stm32f072rb_stm32f0discovery
    stm32f100c8_microboard
    stm32f100c8_vaisala_rs41
    stm32f100cb_tempsensor
    stm32f100rb_stm32vldiscovery
    stm32f100rc_solertegiard
    stm32f100xb_generic
    stm32f103c8_bluepill
    stm32f103cb_als_mainboard_rev2
    stm32f103ve_mp3v2
    stm32f103ve_strive_mini
    stm32f103xb_generic
    stm32f103ze_redbull_v2
    stm32f103ze_stm3210e-eval
    stm32f205_generic
    stm32f205rc_skyward_stormtrooper
    stm32f205rg_sony-newman
    stm32f207ig_stm3220g-eval
    stm32f207ze_als_camboard
    stm32f207zg_ethboard_v2
    stm32f207zg_nucleo
    stm32f303vc_stm32f3discovery
    stm32f401re_nucleo
    stm32f401vc_stm32f4discovery
    stm32f407vg_bitsboard
    stm32f407vg_stm32f4discovery
    stm32f407vg_thermal_test_chip
    stm32f411ce_blackpill
    stm32f411re_nucleo
    stm32f415vg_st25dvdiscovery
    stm32f429zi_oledboard2
    stm32f429zi_skyward_anakin
    stm32f429zi_skyward_homeone
    stm32f429zi_stm32f4discovery
    stm32f469ni_stm32f469i-disco
    stm32f746zg_nucleo
    stm32f765ii_marco_ram_board
    stm32f767zi_nucleo
    stm32f769ni_discovery
    stm32h503rb_generic
    stm32h723zg_nucleo
    stm32h753xi_eval
    stm32h755zi_nucleo
    stm32l010rb_nucleo
    stm32l053r8_nucleo
    stm32l151c8_als_mainboard
    stm32l152re_nucleo
    stm32l476rg_nucleo
    stm32l4r9zi_sensortile
    stm32u535cb_rrc
    stm32u585ci_generic
)

# Target board option, this also implicitly select the target architecture
set(MIOSIX_OPT_BOARD NOT_SET CACHE STRING "Target board")
set_property(CACHE MIOSIX_OPT_BOARD PROPERTY STRINGS ${MIOSIX_BOARDS})
if(MIOSIX_OPT_BOARD STREQUAL NOT_SET)
    miosix_expected_item_in_list("You must specify a target board with MIOSIX_OPT_BOARD" "${MIOSIX_BOARDS}")
elseif(NOT MIOSIX_OPT_BOARD IN_LIST MIOSIX_BOARDS)
    miosix_expected_item_in_list("Selected board '${MIOSIX_OPT_BOARD}' is not in the MIOSIX_BOARDS list" "${MIOSIX_BOARDS}")
endif()

# Optimization flags are set with CMAKE_<LANG>_FLAGS_<CONFIG>
# The default values are in the toolchain file

# Directory with header files for this board
set(MIOSIX_BOARD_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/board/${MIOSIX_OPT_BOARD})

# Include the board specific CMakeLists
include(${MIOSIX_BOARD_INC}/CMakeLists.txt)

# Separate linker script names from associated compiler options
set(LINKER_SCRIPTS_ONLY_LIST "${MIOSIX_LINKER_SCRIPT_LIST}")
list(FILTER LINKER_SCRIPTS_ONLY_LIST INCLUDE REGEX ".*\.ld$")

# Normalize linker script option
set(MIOSIX_LINKER_SCRIPT "${MIOSIX_DEFAULT_LINKER_SCRIPT}" CACHE STRING "Linker script")
set_property(CACHE MIOSIX_LINKER_SCRIPT PROPERTY STRINGS ${MIOSIX_LINKER_SCRIPT_LIST})
if(NOT MIOSIX_LINKER_SCRIPT MATCHES ".*\.ld$")
    message(FATAL_ERROR "MIOSIX_LINKER_SCRIPT '${MIOSIX_LINKER_SCRIPT}' is not a valid linker script because it does not end with .ld")
endif()
list(FIND MIOSIX_LINKER_SCRIPT_LIST "${MIOSIX_LINKER_SCRIPT}" LINKER_SCRIPT_IDX)
if(LINKER_SCRIPT_IDX EQUAL -1)
    # If there is no cached value, or the cached value is not in the list, complain
    miosix_expected_item_in_list("Invalid linker script '${MIOSIX_LINKER_SCRIPT}' specified with MIOSIX_LINKER_SCRIPT" "${LINKER_SCRIPTS_ONLY_LIST}")
endif()
# Set compilation options based on linker scripts
list(SUBLIST MIOSIX_LINKER_SCRIPT_LIST ${LINKER_SCRIPT_IDX} -1 LINKER_SCRIPT_OPTIONS)
foreach(OPTION IN LISTS LINKER_SCRIPT_OPTIONS)
    if(OPTION EQUAL "${MIOSIX_LINKER_SCRIPT}")
        # first iteration, ignore
        continue()
    endif()
    if(OPTION MATCHES ".*\.ld$")
        break()
    endif()
    list(APPEND MIOSIX_BOARD_C_FLAGS "${OPTION}")
    list(APPEND MIOSIX_BOARD_CXX_FLAGS "${OPTION}")
    message(STATUS "Linker script '${MIOSIX_LINKER_SCRIPT}' adds C/CXX flag ${OPTION}")
endforeach()

# Normalize board variant option
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
