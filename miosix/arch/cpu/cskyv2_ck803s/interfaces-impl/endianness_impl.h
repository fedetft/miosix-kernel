/***************************************************************************
 *   CK803S (C-SKY V2) endianness for modern Miosix. Little-endian (-EL).   *
 *   GPL v2+ with the Miosix linking exception.                            *
 ***************************************************************************/

#pragma once

#ifndef MIOSIX_BIG_ENDIAN
//ck803s built little-endian (-EL)
#define MIOSIX_LITTLE_ENDIAN
#endif //MIOSIX_BIG_ENDIAN

namespace miosix {

inline unsigned short swapBytes16(unsigned short x)
{
    return static_cast<unsigned short>((x>>8) | (x<<8));
}

inline unsigned int swapBytes32(unsigned int x)
{
    return __builtin_bswap32(x);
}

inline unsigned long long swapBytes64(unsigned long long x)
{
    return __builtin_bswap64(x);
}

} //namespace miosix
