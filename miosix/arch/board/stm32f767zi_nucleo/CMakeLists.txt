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
set(MIOSIX_ARCH_INC ${MIOSIX_KPATH}/arch/chip/stm32f7)

# The stm32f767 has the double precision FPU, so we will build for m7
set(MIOSIX_CPU_FLAGS -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16)

# Linker script type
set(MIOSIX_LINKER_SCRIPT_LIST
    stm32_2m+384k_ram.ld
    stm32_2m+384k_ram_processes.ld
)
set(MIOSIX_DEFAULT_LINKER_SCRIPT stm32_2m+384k_ram.ld)

# Uncommenting __ENABLE_XRAM enables the initialization of the external
# 8MB SDRAM memory. Do not uncomment this even if you don't use a linker
# script that requires it, as it is used for the LCD framebuffer.
option(MIOSIX_ENABLE_XRAM "Enables the initialization of the external 16MB SDRAM memory" ON)
if(MIOSIX_ENABLE_XRAM)
    set(XRAM -D__ENABLE_XRAM) # TODO: Change to an always defined flag
else()
    message(NOTICE "You have disabled the XRAM, make sure that the selected linker script does not use it")
endif()

# Select architecture specific files
set(MIOSIX_ARCH_SRC
    ${MIOSIX_BOARD_INC}/boot.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/bsp.cpp
)

# Add a #define to allow querying board name
list(APPEND MIOSIX_C_FLAGS -D_BOARD_STM32F767ZI_NUCLEO)
list(APPEND MIOSIX_CXX_FLAGS -D_BOARD_STM32F767ZI_NUCLEO)

# Specify a custom flash command
# This is the program that is invoked when the program-<target_name> target is
# built. Use <binary> or <hex> as placeolders, they will be replaced by the
# build systems with the binary or hex file path repectively.
# If a command is not specified, the build system will fallback to st-flash
# set(PROGRAM_CMDLINE st-flash --connect-under-reset --reset write <binary> 0x8000000)
