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
set(MIOSIX_BOARD_INC ${MIOSIX_KPATH}/arch/${MIOSIX_ARCH_NAME}/${MIOSIX_OPT_BOARD})

# The user can set a custom path for board_settings.h
set(MIOSIX_CUSTOM_BOARD_SETTINGS_INC "" CACHE PATH "Include directory for custom board_settings.h")
mark_as_advanced(MIOSIX_CUSTOM_BOARD_SETTINGS_INC)
if(MIOSIX_CUSTOM_BOARD_SETTINGS_INC)
    set(MIOSIX_BOARD_SETTINGS_INC ${MIOSIX_CUSTOM_BOARD_SETTINGS_INC})
    message(NOTICE "You have set a custom path for board_settings.h")
else()
    set(MIOSIX_BOARD_SETTINGS_INC ${MIOSIX_KPATH}/config/arch/${MIOSIX_ARCH_NAME}/${MIOSIX_OPT_BOARD})
endif()

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
if(NOT MIOSIX_LINKER_SCRIPT IN_LIST MIOSIX_LINKER_SCRIPT_LIST)
    # If there is no cached value, or the cached value is not in the list, set a default value
    set(MIOSIX_LINKER_SCRIPT stm32_1m+192k_rom.ld CACHE STRING "Linker script" FORCE)
endif()
set_property(CACHE MIOSIX_LINKER_SCRIPT PROPERTY STRINGS ${MIOSIX_LINKER_SCRIPT_LIST})

# This causes the interrupt vector table to be relocated in SRAM, must be
# uncommented when using the ram linker script
option(MIOSIX_SRAM_BOOT "This causes the interrupt vector table to be relocated in SRAM" OFF)
if(MIOSIX_SRAM_BOOT)
    set(SRAM_BOOT -DVECT_TAB_SRAM) # TODO: Change to an always defined flag
elseif(MIOSIX_LINKER_SCRIPT STREQUAL ${MIOSIX_BOARD_INC}/stm32_1m+192k_ram.ld)
    message(WARNING "You are relocating the interrupt vector table in SRAM but you are not using the ram linker script")
endif()

# Select HSE clock frequency (external clock on board, fixed)
set(MIOSIX_HSE_VALUE -DHSE_VALUE=8000000)

# Select clock frequency (HSE_VALUE is the xtal on board, fixed)
set(MIOSIX_SYSCLK_FREQ_LIST
    -DSYSCLK_FREQ_168MHz=168000000
    -DSYSCLK_FREQ_100MHz=100000000
    -DSYSCLK_FREQ_84MHz=84000000
)
if(NOT MIOSIX_SYSCLK_FREQ IN_LIST MIOSIX_SYSCLK_FREQ_LIST)
    # If there is no cached value, or the cached value is not in the list, set a default value
    set(MIOSIX_SYSCLK_FREQ -DSYSCLK_FREQ_168MHz=168000000 CACHE STRING "Clock frenquency" FORCE)
endif()
set_property(CACHE MIOSIX_SYSCLK_FREQ PROPERTY STRINGS ${MIOSIX_SYSCLK_FREQ_LIST})

# Select architecture specific files
set(MIOSIX_ARCH_SRC
    ${MIOSIX_BOARD_INC}/core/stage_1_boot.cpp
    ${MIOSIX_KPATH}/arch/common/drivers/stm32f2_f4_i2c.cpp
    ${MIOSIX_KPATH}/arch/common/drivers/stm32_hardware_rng.cpp
    ${MIOSIX_KPATH}/arch/common/drivers/servo_stm32.cpp
    ${MIOSIX_BOARD_INC}/drivers/rtc.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/deep_sleep.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/bsp.cpp
)

# Add a #define to allow querying board name
list(APPEND MIOSIX_C_FLAGS -D_BOARD_STM32F4DISCOVERY)
list(APPEND MIOSIX_CXX_FLAGS -D_BOARD_STM32F4DISCOVERY)

# Specify a custom flash command
# This is the program that is invoked when the program-<target_name> target is
# built. Use <binary> or <hex> as placeolders, they will be replaced by the
# build systems with the binary or hex file path repectively.
# If a command is not specified, the build system will fallback to st-flash
# set(PROGRAM_CMDLINE st-flash --connect-under-reset --reset write <binary> 0x8000000)
