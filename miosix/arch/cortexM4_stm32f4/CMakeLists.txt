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
set(MIOSIX_CPU_MICROARCH armv7m)

# Base directory with common header files for this board
set(MIOSIX_ARCH_INC ${MIOSIX_KPATH}/arch/${MIOSIX_ARCH_NAME}/common)

# Include board specific options
include(${MIOSIX_KPATH}/arch/${MIOSIX_ARCH_NAME}/${MIOSIX_OPT_BOARD}/CMakeLists.txt)

# Automatically transform the architecture name into upper cases
string(TOUPPER ${MIOSIX_ARCH_NAME} MIOSIX_ARCH_NAME_UPPER)

# Set the path to the multilib directory for this architecture
set(MIOSIX_MULTILIB_PATH thumb/v7e-m+fp/hard)

# Select appropriate compiler flags for both ASM/C/C++/linker
set(MIOSIX_CPU_FLAGS -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)
set(MIOSIX_A_FLAGS ${MIOSIX_CPU_FLAGS})
list(APPEND MIOSIX_C_FLAGS
    -D_ARCH_${MIOSIX_ARCH_NAME_UPPER} ${XRAM}
    ${MIOSIX_CPU_FLAGS} -c
)
list(APPEND MIOSIX_CXX_FLAGS
    -D_ARCH_${MIOSIX_ARCH_NAME_UPPER} ${XRAM}
    ${MIOSIX_CPU_FLAGS} ${MIOSIX_OPT_EXCEPT} -c
)
set(MIOSIX_L_FLAGS
    ${MIOSIX_CPU_FLAGS} -Wl,--gc-sections
    -Wl,-T${MIOSIX_BOARD_INC}/${MIOSIX_LINKER_SCRIPT} ${MIOSIX_OPT_EXCEPT} -nostdlib
)

# Select architecture specific files
list(APPEND MIOSIX_ARCH_SRC
    ${MIOSIX_KPATH}/arch/cpu/armv7m/interfaces-impl/cpu.cpp
    ${MIOSIX_KPATH}/arch/cpu/common/cortexMx_interrupts.cpp
    ${MIOSIX_KPATH}/arch/common/mpu/cortexMx_mpu.cpp
    ${MIOSIX_KPATH}/arch/cpu/common/cortexMx_userspace.cpp
    ${MIOSIX_KPATH}/arch/common/os_timer/stm32_32bit_os_timer.cpp
    ${MIOSIX_KPATH}/arch/common/os_timer/arm_systick_os_timer.cpp
    ${MIOSIX_KPATH}/arch/common/sleep/cortexMx_sleep.cpp
    ${MIOSIX_KPATH}/arch/common/drivers/stm32_gpio.cpp
    ${MIOSIX_KPATH}/arch/common/drivers/stm32f1_f2_f4_serial.cpp
    ${MIOSIX_KPATH}/arch/common/drivers/stm32_serial_common.cpp
    ${MIOSIX_KPATH}/arch/common/drivers/dcc.cpp
    ${MIOSIX_KPATH}/arch/common/drivers/sd_stm32f2_f4_f7.cpp
    ${MIOSIX_ARCH_INC}/interfaces-impl/delays.cpp
    ${MIOSIX_ARCH_INC}/interfaces-impl/poweroff.cpp
    ${MIOSIX_KPATH}/arch/common/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
)
