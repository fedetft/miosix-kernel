/***************************************************************************
 *   Copyright (C) 2026 by Radu Raul                                       *
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
#include "miosix.h"

namespace miosix
{

class CRC32Calculator
{
public:
    CRC32Calculator() 
    {
        // GPT / Standard Ethernet starts with a seed of 0xFFFFFFFF
        crc = 0xFFFFFFFF; 
    }

    void addBytes(const uint8_t* data, size_t length)
    {
        for (size_t i = 0; i < length; i++)
        {
            crc ^= data[i];
            for (int bit = 0; bit < 8; bit++)
            {
                // GPT uses the reflected (LSB-first) polynomial 0xEDB88320
                if (crc & 1) {
                    crc = (crc >> 1) ^ 0xEDB88320;
                } else {
                    crc = (crc >> 1);
                }
            }
        }
    }

    uint32_t finalizeCRC32() 
    {
        // Return the inverted result (Final XOR 0xFFFFFFFF)
        return crc ^ 0xFFFFFFFF; 
    }

    void reset()
    {
        crc = 0xFFFFFFFF;
    }

private: 
    uint32_t crc;
};

}