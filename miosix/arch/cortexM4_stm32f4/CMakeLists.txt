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

# Base directory with common header files for this board
set(MIOSIX_ARCH_INC ${MIOSIX_KPATH}/arch/${MIOSIX_ARCH_NAME}/common)

# Include board specific options
include(${MIOSIX_KPATH}/arch/${MIOSIX_ARCH_NAME}/${MIOSIX_OPT_BOARD}/CMakeLists.txt)

# Automatically transform the architecture name into upper cases
string(TOUPPER ${MIOSIX_ARCH_NAME} MIOSIX_ARCH_NAME_UPPER)

# Select appropriate compiler flags for both ASM/C/C++/linker
set(MIOSIX_CPU_FLAGS -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)
set(MIOSIX_A_FLAGS ${MIOSIX_CPU_FLAGS})
list(APPEND MIOSIX_CFLAGS
    -D_ARCH_${MIOSIX_ARCH_NAME_UPPER} ${MIOSIX_HSE_VALUE} ${MIOSIX_SYSCLK_FREQ} ${XRAM}
    ${SRAM_BOOT} ${MIOSIX_CPU_FLAGS} -c
)
list(APPEND MIOSIX_CXX_FLAGS
    -D_ARCH_${MIOSIX_ARCH_NAME_UPPER} ${MIOSIX_HSE_VALUE} ${MIOSIX_SYSCLK_FREQ} ${XRAM}
    ${SRAM_BOOT} ${MIOSIX_CPU_FLAGS} ${OPT_EXCEPT} -c
)
set(MIOSIX_L_FLAGS
    ${MIOSIX_CPU_FLAGS} -Wl,--gc-sections,-Map,main.map # TODO: Check main.map
    -Wl,-T${MIOSIX_BOARD_INC}/${MIOSIX_LINKER_SCRIPT} ${OPT_EXCEPT} -nostdlib
)

# Select architecture specific files
list(APPEND MIOSIX_ARCH_SRC
    ${MIOSIX_KPATH}/arch/common/core/interrupts_cortexMx.cpp
    ${MIOSIX_KPATH}/arch/common/core/mpu_cortexMx.cpp
    ${MIOSIX_KPATH}/arch/common/drivers/serial_stm32.cpp
    ${MIOSIX_KPATH}/arch/common/drivers/dcc.cpp
    ${MIOSIX_ARCH_INC}/interfaces-impl/portability.cpp
    ${MIOSIX_ARCH_INC}/interfaces-impl/delays.cpp
    ${MIOSIX_KPATH}/arch/common/drivers/stm32_gpio.cpp
    ${MIOSIX_KPATH}/arch/common/drivers/sd_stm32f2_f4_f7.cpp
    ${MIOSIX_KPATH}/arch/common/core/stm32_32bit_os_timer.cpp
    ${MIOSIX_KPATH}/arch/common/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
)
