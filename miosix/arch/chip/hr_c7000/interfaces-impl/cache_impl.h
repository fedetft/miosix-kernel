/***************************************************************************
 *   HR_C7000 (CK803S) cache interface for modern Miosix.                   *
 *   GPL v2+ with the Miosix linking exception.                            *
 *                                                                          *
 *   CK803S on the HD2 has no data cache to manage for DMA coherency, so    *
 *   these are no-ops (same as the lpc2000 chip).                           *
 ***************************************************************************/

#pragma once

namespace miosix {

inline void markBufferBeforeDmaWrite(const void *buffer, int size) {}
inline void markBufferAfterDmaRead(void *buffer, int size) {}
inline void IRQenableCache() {}

} //namespace miosix
