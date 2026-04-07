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
set(MIOSIX_CHIP_INC ${MIOSIX_KPATH}/arch/chip/stm32f4)

# Linker script options:
# 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
# 2) Code + stack + heap in internal RAM (file *_ram.ld)
# 3) Same as 1) but space has been reserved for a process pool, allowing
#    to configure the kernel with "#define WITH_PROCESSES"
set(MIOSIX_LINKER_SCRIPT_LIST
    stm32_1m+192k_rom.ld
    stm32_1m+192k_ram.ld
    stm32_1m+192k_rom_processes.ld
)
set(MIOSIX_DEFAULT_LINKER_SCRIPT stm32_1m+192k_rom.ld)

# Select architecture specific files
set(MIOSIX_BOARD_SRC
    ${MIOSIX_BOARD_INC}/boot.cpp
    ${MIOSIX_KPATH}/arch/drivers/stm32f2_f4_i2c.cpp
    ${MIOSIX_KPATH}/arch/drivers/stm32_hardware_rng.cpp
    ${MIOSIX_KPATH}/arch/drivers/servo_stm32.cpp
    ${MIOSIX_KPATH}/arch/drivers/stm32f407_rtc.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/sleep.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/bsp.cpp
)

# Add a #define to allow querying board name
set(MIOSIX_BOARD_C_FLAGS -D_BOARD_STM32F4DISCOVERY)
set(MIOSIX_BOARD_CXX_FLAGS -D_BOARD_STM32F4DISCOVERY)

# Specify a custom flash command
# This is the program that is invoked when the program-<target_name> target is
# built. Use <binary> or <hex> as placeolders, they will be replaced by the
# build systems with the binary or hex file path repectively.
# If a command is not specified, the build system will fallback to st-flash
# set(PROGRAM_CMDLINE st-flash --connect-under-reset --reset write <binary> 0x8000000)
