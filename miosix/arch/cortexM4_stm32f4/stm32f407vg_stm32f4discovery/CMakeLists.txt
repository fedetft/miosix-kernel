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
set(BOARD_INC ${KPATH}/arch/${ARCH_NAME}/${MIOSIX_OPT_BOARD})

# The user can set a custom path for board_settings.h
set(MIOSIX_BOARD_CONFIG_INC ${KPATH}/config/arch/${ARCH_NAME}/${MIOSIX_OPT_BOARD} CACHE PATH "Include directory for board_settings.h")

# Linker script options:
# 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
# 2) Code + stack + heap in internal RAM (file *_ram.ld)
# 3) Same as 1) but space has been reserved for a process pool, allowing
#    to configure the kernel with "#define WITH_PROCESSES"
set(MIOSIX_LINKER_SCRIPT ${BOARD_INC}/stm32_1m+192k_rom.ld CACHE FILEPATH "Linker script")
# set(MIOSIX_LINKER_SCRIPT ${BOARD_INC}/stm32_1m+192k_ram.ld CACHE FILEPATH "Linker script")
# set(MIOSIX_LINKER_SCRIPT ${BOARD_INC}/stm32_1m+192k_rom_processes.ld CACHE FILEPATH "Linker script")

# This causes the interrupt vector table to be relocated in SRAM, must be
# uncommented when using the ram linker script
option(MIOSIX_SRAM_BOOT "This causes the interrupt vector table to be relocated in SRAM" OFF)
if(MIOSIX_SRAM_BOOT)
    set(SRAM_BOOT -DVECT_TAB_SRAM)
elseif(MIOSIX_LINKER_SCRIPT STREQUAL ${BOARD_INC}/stm32_1m+192k_ram.ld)
    message(WARNING "You are relocating the interrupt vector table in SRAM but you are not using the ram linker script")
endif()

# Select HSE clock frequency (external clock on board, fixed)
set(MIOSIX_HSE_VALUE -DHSE_VALUE=8000000 CACHE STRING "HSE clock frequency (external clock on board, fixed)")

# Select clock frequency (HSE_VALUE is the xtal on board, fixed)
set(MIOSIX_SYSCLK_FREQ -DSYSCLK_FREQ_168MHz=168000000 CACHE STRING "Clock frenquency")
# set(SYSCLK_FREQ -DSYSCLK_FREQ_100MHz=100000000 CACHE STRING "Clock frenquency")
# set(SYSCLK_FREQ -DSYSCLK_FREQ_84MHz=84000000 CACHE STRING "Clock frenquency")
set_property(CACHE MIOSIX_SYSCLK_FREQ PROPERTY STRINGS
    -DSYSCLK_FREQ_168MHz=168000000
    -DSYSCLK_FREQ_100MHz=100000000
    -DSYSCLK_FREQ_84MHz=84000000
)

# Select architecture specific files
set(ARCH_SRC
    ${BOARD_INC}/core/stage_1_boot.cpp
    ${KPATH}/arch/common/drivers/stm32f2_f4_i2c.cpp
    ${KPATH}/arch/common/drivers/stm32_hardware_rng.cpp
    ${KPATH}/arch/common/drivers/servo_stm32.cpp
    ${BOARD_INC}/drivers/rtc.cpp
    ${BOARD_INC}/interfaces-impl/deep_sleep.cpp
    ${BOARD_INC}/interfaces-impl/bsp.cpp
)

# Add a #define to allow querying board name
list(APPEND CFLAGS_BASE -D_BOARD_STM32F4DISCOVERY)
list(APPEND CXXFLAGS_BASE -D_BOARD_STM32F4DISCOVERY)

# Specify a custom flash command
# This is the program that is invoked when the program-<target_name> target is
# built. Use <binary> or <hex> as placeolders, they will be replaced by the
# build systems with the binary or hex file path repectively.
# If a command is not specified, the build system will fallback to st-flash
set(PROGRAM_CMDLINE qstlink2 -cqewV <binary>)
