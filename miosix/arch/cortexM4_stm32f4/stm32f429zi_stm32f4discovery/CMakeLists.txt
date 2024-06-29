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

# Directory with header files for this board
set(BOARD_INC ${KPATH}/arch/${ARCH_NAME}/${OPT_BOARD})

# Directory with board setting header file
set(BOARD_CONFIG_INC ${KPATH}/config/arch/${ARCH_NAME}/${OPT_BOARD})

# Include user configurable options
include(${BOARD_CONFIG_INC}/board_settings.cmake)

# Select linker script
# set(LINKER_SCRIPT already selected in board options)

# Select architecture specific files
set(ARCH_SRC
    ${BOARD_INC}/core/stage_1_boot.cpp
    ${KPATH}/arch/common/drivers/stm32f2_f4_i2c.cpp
    ${KPATH}/arch/common/drivers/stm32_hardware_rng.cpp
    ${BOARD_INC}/interfaces-impl/bsp.cpp
)

# Add a #define to allow querying board name
list(APPEND CFLAGS_BASE -D_BOARD_STM32F429ZI_STM32F4DISCOVERY)
list(APPEND CXXFLAGS_BASE -D_BOARD_STM32F429ZI_STM32F4DISCOVERY)

# Specify a custom flash command
# This is the program that is invoked when the program-<target_name> target is
# built. Use <binary> or <hex> as placeolders, they will be replaced by the
# build systems with the binary or hex file path repectively.
# If a command is not specified, the build system will fallback to st-flash
set(PROGRAM_CMDLINE qstlink2 -cqewV <binary>)
