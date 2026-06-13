/***************************************************************************
 *   Board-specific inline helpers for the Ailunce HD2 (HR_C7000/CK803S).   *
 *   GPL v2+ with the Miosix linking exception.                            *
 *                                                                          *
 *   ledOn/ledOff drive the GREEN LED (PTB0, active-high) — the Phase-1     *
 *   bring-up signal (no serial console yet). GPIOB DR @ 0x14100000.        *
 ***************************************************************************/

#pragma once

namespace miosix {

inline void ledOn()
{
    *reinterpret_cast<volatile unsigned int*>(0x14100000u) |= (1u<<0);  //PTB0 high
}

inline void ledOff()
{
    *reinterpret_cast<volatile unsigned int*>(0x14100000u) &= ~(1u<<0); //PTB0 low
}

} //namespace miosix
