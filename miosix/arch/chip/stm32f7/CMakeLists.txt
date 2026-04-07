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

# CPU microarchitecture
set(MIOSIX_CPU_INC ${MIOSIX_KPATH}/arch/cpu/armv7m)

# Set the path to the multilib directory for this architecture (used by clang)
set(MIOSIX_MULTILIB_PATH thumb/v7e-m+dp/hard)

# Select appropriate compiler flags for both C/C++/linker
set(MIOSIX_CHIP_C_FLAGS -D_CHIP_STM32F7 ${XRAM})
set(MIOSIX_CHIP_CXX_FLAGS -D_CHIP_STM32F7 ${XRAM})

# Select architecture specific files
set(MIOSIX_CHIP_SRC
    ${MIOSIX_KPATH}/arch/cpu/armv7m/interfaces-impl/cpu.cpp
    ${MIOSIX_KPATH}/arch/cpu/common/cortexMx_interrupts.cpp
    ${MIOSIX_KPATH}/arch/drivers/mpu/cortexMx_mpu.cpp
    ${MIOSIX_KPATH}/arch/cpu/common/cortexMx_userspace.cpp
    ${MIOSIX_KPATH}/arch/drivers/os_timer/stm32_32bit_os_timer.cpp
    ${MIOSIX_KPATH}/arch/drivers/sleep/cortexMx_sleep.cpp
    ${MIOSIX_KPATH}/arch/drivers/cache/cortexMx_cache.cpp
    ${MIOSIX_KPATH}/arch/drivers/gpio/stm32_gpio.cpp
    ${MIOSIX_KPATH}/arch/drivers/serial/stm32f7_serial.cpp
    ${MIOSIX_KPATH}/arch/drivers/serial/stm32_serial_common.cpp
    ${MIOSIX_KPATH}/arch/drivers/dcc.cpp
    ${MIOSIX_KPATH}/arch/drivers/sdmmc/sd_stm32f2_f4_f7.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/delays.cpp
    ${MIOSIX_CHIP_INC}/interfaces-impl/poweroff.cpp
    ${MIOSIX_KPATH}/arch/CMSIS/Device/ST/STM32F7xx/Source/Templates/system_stm32f7xx.cpp
)
