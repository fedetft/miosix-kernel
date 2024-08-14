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

# Linker script type, there are three options
# 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
#    the most common choice, available for all microcontrollers
# 2) Code in FLASH, stack + heap in external RAM (file *8m_xram.ld)
#    You must uncomment -D__ENABLE_XRAM below in this case.
# 3) Code in FLASH, stack + heap in external RAM (file *6m_xram.ld)
#    Same as above, but leaves the upper 2MB of RAM for the LCD.
set(MIOSIX_LINKER_SCRIPT_LIST
    stm32_2m+256k_rom.ld
    stm32_2m+256k_rom_processes.ld
    stm32_2m+8m_xram.ld
    stm32_2m+6m_xram.ld
)
if(NOT MIOSIX_LINKER_SCRIPT IN_LIST MIOSIX_LINKER_SCRIPT_LIST)
    # If there is no cached value, or the cached value is not in the list, set a default value
    set(MIOSIX_LINKER_SCRIPT stm32_2m+6m_xram.ld CACHE STRING "Linker script" FORCE)
endif()
set_property(CACHE MIOSIX_LINKER_SCRIPT PROPERTY STRINGS ${MIOSIX_LINKER_SCRIPT_LIST})

# Uncommenting __ENABLE_XRAM enables the initialization of the external
# 8MB SDRAM memory. Do not uncomment this even if you don't use a linker
# script that requires it, as it is used for the LCD framebuffer.
option(MIOSIX_ENABLE_XRAM "Enables the initialization of the external 16MB SDRAM memory" ON)
if(MIOSIX_ENABLE_XRAM)
    set(XRAM -D__ENABLE_XRAM) # TODO: Change to an always defined flag
else()
    message(NOTICE "You have disabled the XRAM, make sure that the selected linker script does not use it")
endif()

# Select HSE clock frequency (external clock on board, fixed)
set(MIOSIX_HSE_VALUE -DHSE_VALUE=8000000)

# Select clock frequency.
# Warning: due to a limitation in the PLL, it is not possible to generate
# a precise 48MHz output when running the core at 180MHz. If 180MHz is
# chosen the SDIO and RNG will run ~6% slower (45MHz insteand of 48MHz)
set(MIOSIX_SYSCLK_FREQ_LIST
    -DSYSCLK_FREQ_180MHz=180000000
    -DSYSCLK_FREQ_168MHz=168000000
    -DSYSCLK_FREQ_100MHz=100000000
)
if(NOT MIOSIX_SYSCLK_FREQ IN_LIST MIOSIX_SYSCLK_FREQ_LIST)
    # If there is no cached value, or the cached value is not in the list, set a default value
    set(MIOSIX_SYSCLK_FREQ -DSYSCLK_FREQ_168MHz=168000000 CACHE STRING "Clock frenquency" FORCE)
endif()
set_property(CACHE MIOSIX_SYSCLK_FREQ PROPERTY STRINGS ${MIOSIX_SYSCLK_FREQ_LIST})

if(MIOSIX_SYSCLK_FREQ STREQUAL -DSYSCLK_FREQ_180MHz=180000000)
    message(NOTICE "The clock frequency has been set to 180MHz, the SDIO and RNG will run ~6% slower (45MHz insteand of 48MHz)")
endif()

# Select architecture specific files
set(MIOSIX_ARCH_SRC
    ${MIOSIX_BOARD_INC}/core/stage_1_boot.cpp
    ${MIOSIX_KPATH}/arch/common/drivers/stm32f2_f4_i2c.cpp
    ${MIOSIX_KPATH}/arch/common/drivers/stm32_hardware_rng.cpp
    ${MIOSIX_BOARD_INC}/interfaces-impl/bsp.cpp
)

# Add a #define to allow querying board name
list(APPEND MIOSIX_C_FLAGS -D_BOARD_STM32F429ZI_STM32F4DISCOVERY)
list(APPEND MIOSIX_CXX_FLAGS -D_BOARD_STM32F429ZI_STM32F4DISCOVERY)

# Specify a custom flash command
# This is the program that is invoked when the program-<target_name> target is
# built. Use <binary> or <hex> as placeolders, they will be replaced by the
# build systems with the binary or hex file path repectively.
# If a command is not specified, the build system will fallback to st-flash
# set(PROGRAM_CMDLINE st-flash --connect-under-reset --reset write <binary> 0x8000000)
