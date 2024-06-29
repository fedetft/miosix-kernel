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

set(BOARD_NAME stm32f407vg_stm32f4discovery)
set(ARCH_NAME cortexM4_stm32f4)

# Base directories with header files for this board
set(ARCH_PATH ${KPATH}/arch/${ARCH_NAME}/common)
set(BOARD_PATH ${KPATH}/arch/${ARCH_NAME}/${BOARD_NAME})
set(BOARD_CONFIG_PATH ${KPATH}/config/arch/${ARCH_NAME}/${BOARD_NAME})

# Linker script options:
# 1) Code in FLASH, stack + heap in internal RAM (file *_rom.ld)
# 2) Code + stack + heap in internal RAM (file *_ram.ld)
# 3) Same as 1) but space has been reserved for a process pool, allowing
#    to configure the kernel with "#define WITH_PROCESSES"
set(LINKER_SCRIPT ${BOARD_PATH}/stm32_1m+192k_rom.ld)
# set(LINKER_SCRIPT ${BOARD_PATH}/stm32_1m+192k_ram.ld)
# set(LINKER_SCRIPT ${BOARD_PATH}/stm32_1m+192k_rom_processes.ld)

# set(SRAM_BOOT -DVECT_TAB_SRAM)
# This causes the interrupt vector table to be relocated in SRAM, must be
# uncommented when using the ram linker script

# Select clock frequency (HSE_VALUE is the xtal on board, fixed)
set(CLOCK_FREQ -DHSE_VALUE=8000000 -DSYSCLK_FREQ_168MHz=168000000)
# set(CLOCK_FREQ -DHSE_VALUE=8000000 -DSYSCLK_FREQ_100MHz=100000000)
# set(CLOCK_FREQ -DHSE_VALUE=8000000 -DSYSCLK_FREQ_84MHz=84000000)

# C++ Exception/rtti support disable flags.
# To save code size if not using C++ exceptions (nor some STL code which
# implicitly uses it) uncomment this option.
# -D__NO_EXCEPTIONS is used by Miosix to know if exceptions are used.
# set(OPT_EXCEPT -fno-exceptions -fno-rtti -D__NO_EXCEPTIONS)

# Specify a custom flash command
# This is the program that is invoked when the program-<target_name> target is
# built. Use <binary> or <hex> as placeolders, they will be replaced by the
# build systems with the binary or hex file path repectively.
# If a command is not specified, the build system will fallback to st-flash
set(PROGRAM_CMDLINE qstlink2 -cqewV <binary>)

# Basic flags
set(FLAGS_BASE -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16)

# Flags for ASM and linker
set(AFLAGS ${FLAGS_BASE})
set(LFLAGS ${FLAGS_BASE} -Wl,--gc-sections,-Map,main.map -Wl,-T${LINKER_SCRIPT} ${OPT_EXCEPT} ${OPT_OPTIMIZATION} -nostdlib)

# Flags for C/C++
string(TOUPPER ${ARCH_NAME} ARCH_NAME_UPPER)
set(CFLAGS
    -D_BOARD_STM32F4DISCOVERY -D_MIOSIX_BOARDNAME="${BOARD_NAME}"
    -D_DEFAULT_SOURCE=1 -ffunction-sections -Wall -Werror=return-type
    -D_ARCH_${ARCH_NAME_UPPER}
    ${CLOCK_FREQ} ${XRAM} ${SRAM_BOOT} ${FLAGS_BASE} -c
)
set(CXXFLAGS ${CFLAGS} -std=c++14 ${OPT_EXCEPT})

# Select architecture specific files
set(ARCH_SRC
    ${BOARD_PATH}/core/stage_1_boot.cpp

    ${KPATH}/arch/common/drivers/stm32f2_f4_i2c.cpp
    ${KPATH}/arch/common/drivers/stm32_hardware_rng.cpp
    ${KPATH}/arch/common/drivers/servo_stm32.cpp
    ${BOARD_PATH}/drivers/rtc.cpp
    ${BOARD_PATH}/interfaces-impl/deep_sleep.cpp
    ${BOARD_PATH}/interfaces-impl/bsp.cpp

    ${KPATH}/arch/common/core/interrupts_cortexMx.cpp
    ${KPATH}/arch/common/core/mpu_cortexMx.cpp
    ${KPATH}/arch/common/drivers/serial_stm32.cpp
    ${KPATH}/arch/common/drivers/dcc.cpp
    ${ARCH_PATH}/interfaces-impl/portability.cpp
    ${ARCH_PATH}/interfaces-impl/delays.cpp
    ${KPATH}/arch/common/drivers/stm32_gpio.cpp
    ${KPATH}/arch/common/drivers/sd_stm32f2_f4_f7.cpp
    ${KPATH}/arch/common/core/stm32_32bit_os_timer.cpp
    ${KPATH}/arch/common/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
)
