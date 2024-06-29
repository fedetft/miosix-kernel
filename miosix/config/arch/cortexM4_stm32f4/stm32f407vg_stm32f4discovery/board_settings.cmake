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

# C++ Exception/rtti support disable flags.
# To save code size if not using C++ exceptions (nor some STL code which
# implicitly uses it) uncomment this option.
# -D__NO_EXCEPTIONS is used by Miosix to know if exceptions are used.
# set(OPT_EXCEPT -fno-exceptions -fno-rtti -D__NO_EXCEPTIONS)

# Linker script options:
# 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
# 2) Code + stack + heap in internal RAM (file *_ram.ld)
# 3) Same as 1) but space has been reserved for a process pool, allowing
#    to configure the kernel with "#define WITH_PROCESSES"
set(LINKER_SCRIPT ${BOARD_INC}/stm32_1m+192k_rom.ld)
# set(LINKER_SCRIPT ${BOARD_INC}/stm32_1m+192k_ram.ld)
# set(LINKER_SCRIPT ${BOARD_INC}/stm32_1m+192k_rom_processes.ld)

# This causes the interrupt vector table to be relocated in SRAM, must be
# uncommented when using the ram linker script
# set(SRAM_BOOT -DVECT_TAB_SRAM)

# Select clock frequency (HSE_VALUE is the xtal on board, fixed)
set(CLOCK_FREQ -DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000)
# set(CLOCK_FREQ -DHSE_VALUE=8000000 -DSYSCLK_FREQ_100MHz=100000000)
# set(CLOCK_FREQ -DHSE_VALUE=8000000 -DSYSCLK_FREQ_84MHz=84000000)
