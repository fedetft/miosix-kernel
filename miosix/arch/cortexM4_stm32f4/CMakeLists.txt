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
set(ARCH_INC ${KPATH}/arch/${ARCH_NAME}/common)

# Include board specific options
include(${KPATH}/arch/${ARCH_NAME}/${MIOSIX_OPT_BOARD}/CMakeLists.txt)

# Automatically transform the architecture name into upper cases
string(TOUPPER ${ARCH_NAME} ARCH_NAME_UPPER)

# Select appropriate compiler flags for both ASM/C/C++/linker
set(CPU -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)
set(AFLAGS_BASE ${CPU})
list(APPEND CFLAGS_BASE
    -D_ARCH_${ARCH_NAME_UPPER} ${MIOSIX_HSE_VALUE} ${MIOSIX_SYSCLK_FREQ} ${XRAM}
    ${SRAM_BOOT} ${CPU} -c
)
list(APPEND CXXFLAGS_BASE
    -D_ARCH_${ARCH_NAME_UPPER} ${MIOSIX_HSE_VALUE} ${MIOSIX_SYSCLK_FREQ} ${XRAM}
    ${SRAM_BOOT} ${CPU} ${OPT_EXCEPT} -c
)
set(LFLAGS_BASE
    ${CPU} -Wl,--gc-sections,-Map,main.map # TODO: Check main.map
    -Wl,-T${BOARD_INC}/${MIOSIX_LINKER_SCRIPT} ${OPT_EXCEPT} -nostdlib
)

# Select architecture specific files
list(APPEND ARCH_SRC
    ${KPATH}/arch/common/core/interrupts_cortexMx.cpp
    ${KPATH}/arch/common/core/mpu_cortexMx.cpp
    ${KPATH}/arch/common/drivers/serial_stm32.cpp
    ${KPATH}/arch/common/drivers/dcc.cpp
    ${ARCH_INC}/interfaces-impl/portability.cpp
    ${ARCH_INC}/interfaces-impl/delays.cpp
    ${KPATH}/arch/common/drivers/stm32_gpio.cpp
    ${KPATH}/arch/common/drivers/sd_stm32f2_f4_f7.cpp
    ${KPATH}/arch/common/core/stm32_32bit_os_timer.cpp
    ${KPATH}/arch/common/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
)
