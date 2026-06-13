/***************************************************************************
 *   CK803S (C-SKY V2) MPU interface for modern Miosix.                     *
 *   GPL v2+ with the Miosix linking exception.                            *
 ***************************************************************************/

#pragma once

#warning Architecture does not provide an MPU, kernel-level W^X will not be enforced

namespace miosix {

/**
 * \internal
 * The CK803S on the HD2 has no MPU we use, so this is a no-op.
 */
inline void IRQenableMPU() {}

} //namespace miosix
