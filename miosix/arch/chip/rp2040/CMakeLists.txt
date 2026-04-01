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
set(MIOSIX_CPU_INC ${MIOSIX_KPATH}/arch/cpu/armv6m)

# Set the path to the multilib directory for this architecture (used by clang)
set(MIOSIX_MULTILIB_PATH thumb/v6-m/nofp)

# Select appropriate compiler flags for both ASM/C/C++/linker
set(MIOSIX_CPU_FLAGS -mcpu=cortex-m0plus -mthumb)
set(MIOSIX_A_FLAGS
    ${MIOSIX_CPU_FLAGS}
)
list(APPEND MIOSIX_C_FLAGS
    -D_CHIP_RP2040 ${XRAM}
    ${MIOSIX_CPU_FLAGS} -c
)
list(APPEND MIOSIX_CXX_FLAGS
    -D_CHIP_RP2040 ${XRAM}
    ${MIOSIX_CPU_FLAGS} ${MIOSIX_OPT_EXCEPT} -c
)
set(MIOSIX_L_FLAGS
    ${MIOSIX_CPU_FLAGS} -Wl,--gc-sections
    -Wl,-T${MIOSIX_BOARD_INC}/${MIOSIX_LINKER_SCRIPT} ${MIOSIX_OPT_EXCEPT}
    -nostdlib
)

# Select architecture specific files
list(APPEND MIOSIX_ARCH_SRC
    ${MIOSIX_KPATH}/arch/cpu/armv6m/interfaces-impl/cpu.cpp
    ${MIOSIX_KPATH}/arch/cpu/common/cortexMx_interrupts.cpp
    ${MIOSIX_KPATH}/arch/drivers/mpu/cortexMx_mpu.cpp
    ${MIOSIX_KPATH}/arch/cpu/common/cortexMx_userspace.cpp
    ${MIOSIX_KPATH}/arch/drivers/os_timer/rp2040_os_timer.cpp
    ${MIOSIX_KPATH}/arch/drivers/sleep/cortexMx_sleep.cpp
    ${MIOSIX_KPATH}/arch/drivers/serial/arm_pl011_serial.cpp
    ${MIOSIX_KPATH}/arch/drivers/serial/rp2040_serial.cpp
    ${MIOSIX_KPATH}/arch/drivers/spi/arm_pl022_spi.cpp
    ${MIOSIX_KPATH}/arch/drivers/spi/rp2040_spi.cpp
    ${MIOSIX_KPATH}/arch/drivers/rp2040_dma.cpp
    ${MIOSIX_ARCH_INC}/interfaces-impl/delays.cpp
    ${MIOSIX_ARCH_INC}/interfaces-impl/smp.cpp
    ${MIOSIX_ARCH_INC}/interfaces-impl/atomic_ops_smp_impl.cpp
    ${MIOSIX_ARCH_INC}/interfaces-impl/poweroff.cpp
    ${MIOSIX_KPATH}/arch/CMSIS/Device/RaspberryPi/RP2040/Source/system_RP2040.c
)
