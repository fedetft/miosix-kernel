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

# Target board option, this also implicitly select the target architecture
set(MIOSIX_OPT_BOARD NOT_SET CACHE STRING "Target board")
set_property(CACHE MIOSIX_OPT_BOARD PROPERTY STRINGS ${MIOSIX_BOARDS})
if(MIOSIX_OPT_BOARD STREQUAL NOT_SET)
    message(FATAL_ERROR "You must specify a target board with MIOSIX_OPT_BOARD")
elseif(NOT MIOSIX_OPT_BOARD IN_LIST MIOSIX_BOARDS)
    message(FATAL_ERROR "Selected board is not in the MIOSIX_BOARDS list")
endif()

# Optimization flags are set with CMAKE_<LANG>_FLAGS_<CONFIG>
# The default values are in the toolchain file

# C++ Exception/rtti support disable flags.
# To save code size if not using C++ exceptions (nor some STL code which
# implicitly uses it) uncomment this option.
# -D__NO_EXCEPTIONS is used by Miosix to know if exceptions are used.
option(MIOSIX_DISABLE_EXEPTIONS "Disables C++ exceptions/rtti support" OFF)
if(MIOSIX_DISABLE_EXEPTIONS)
    set(OPT_EXCEPT -fno-exceptions -fno-rtti -D__NO_EXCEPTIONS)
endif()

# Auto guess architecture name from board name
if(
    MIOSIX_OPT_BOARD STREQUAL stm32f407vg_stm32f4discovery OR
    MIOSIX_OPT_BOARD STREQUAL stm32f429zi_stm32f4discovery
)
    set(MIOSIX_ARCH_NAME cortexM4_stm32f4)
endif()

# Then, initialize C/C++ flags
set(MIOSIX_CFLAGS
    -D_MIOSIX_BOARDNAME="${MIOSIX_OPT_BOARD}" -D_DEFAULT_SOURCE=1
    -ffunction-sections -Wall -Werror=return-type -g
)
set(MIOSIX_CXX_FLAGS
    -D_MIOSIX_BOARDNAME="${MIOSIX_OPT_BOARD}" -D_DEFAULT_SOURCE=1
    -std=c++14 -ffunction-sections -Wall -Werror=return-type -g
)

# Now include the architecture specific CMakeLists that will:
# - Sets common things for all boards of that specific architecture
# - Includes the board specific CMakeLists file
include(${MIOSIX_KPATH}/arch/${MIOSIX_ARCH_NAME}/CMakeLists.txt)
