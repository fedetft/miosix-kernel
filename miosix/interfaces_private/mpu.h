/***************************************************************************
 *   Copyright (C) 2026 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#pragma once

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file mpu.h
 * This file allows the kernel to configure a Memory Protection Unit to enforce
 * kernel-level W^X memory protection.
 * On some architectures such as ARM, the MPU is also used to configure
 * cacheability.
 * The implementation of this interface should cooperate with the dynamic MPU
 * configuration needed to implement processes that is instead declared in
 * userspace.h
 */

namespace miosix {

/**
 * \internal
 * The kernel calls this function in boot.cpp to enable the MPU
 */
void IRQenableMPU();

} //namespace miosix

/**
 * \}
 */

#include "interfaces-impl/mpu_impl.h"
