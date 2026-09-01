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
#include <string>
#include <cstring>
#include <format>

namespace UUID 
{
   
constexpr size_t UUID_LEN=16; // uuid is 16 bytes in length

class UUID 
{
public:
    constexpr UUID()=default;

    UUID(uint8_t bytes[UUID_LEN]) 
    {
        std::memcpy(this->bytes, bytes, UUID_LEN);
    } 

    static UUID fromBigEndian(uint8_t bytes[UUID_LEN]) 
    {
        UUID uuid;
        // first pack of 4 bytes are in big endian, so we need to reverse them
        uuid.bytes[0]=bytes[3];
        uuid.bytes[1]=bytes[2];
        uuid.bytes[2]=bytes[1];
        uuid.bytes[3]=bytes[0];

        // second pack of 2 bytes are in big endian, so we need to reverse them
        uuid.bytes[4]=bytes[5];
        uuid.bytes[5]=bytes[4];

        // third pack of 2 bytes are in big endian, so we need to reverse them
        uuid.bytes[6]=bytes[7];
        uuid.bytes[7]=bytes[6];

        // the last 8 bytes are single bytes, endianness does not matter 
        // so we can just copy them as is
        std::memcpy(&uuid.bytes[8], &bytes[8], 8);

        return uuid;
    }

    friend bool operator==(const UUID& a, const UUID& b) 
    {
        return memcmp(a.bytes, b.bytes, UUID_LEN)==0;
    }

    friend bool operator<(const UUID& a, const UUID& b) 
    {
        return memcmp(a.bytes, b.bytes, UUID_LEN)<0;
    }

    static consteval char toUpperAndDecimal(const char c) 
    {
        if (c>='a' && c<='f') return c-'a'+10;
        if (c>='0' && c<='9') return c-'0';
        if (c>='A' && c<='F') return c-'A'+10;
        throw "Invalid UUID";
    }

    static consteval void checkDash(const char c) 
    {
        if (c=='-') return;

        throw "Invalid UUID";
    }

    static consteval UUID fromString(const char stringUUID[36]) 
    {
        UUID uuid;
        auto* bytes=uuid.bytes;
        size_t byteIdx=0, i=0;
        for (i=0; i<8; i+=2, byteIdx++) 
        {
            bytes[byteIdx]=toUpperAndDecimal(stringUUID[i])<<4 | toUpperAndDecimal(stringUUID[i+1]);
        }
        checkDash(stringUUID[i]);
        i++; // skip the dash
        
        for (;i<13; i+=2, byteIdx++) 
        {
            bytes[byteIdx]=toUpperAndDecimal(stringUUID[i])<<4 | toUpperAndDecimal(stringUUID[i+1]);
        }

        checkDash(stringUUID[i]);
        i++; // skip the dash again

        for (;i<18; i+=2, byteIdx++) 
        {
            bytes[byteIdx]=toUpperAndDecimal(stringUUID[i])<<4 | toUpperAndDecimal(stringUUID[i+1]);
        }

        checkDash(stringUUID[i]);
        i++; //skip the dash

        for(;i<23; i+=2, byteIdx++) 
        {
            bytes[byteIdx]=toUpperAndDecimal(stringUUID[i])<<4 | toUpperAndDecimal(stringUUID[i+1]);
        }

        checkDash(stringUUID[i]);
        i++; // skip the dash

        for(;i<36; i+=2, byteIdx++) 
        {
            bytes[byteIdx]=toUpperAndDecimal(stringUUID[i])<<4 | toUpperAndDecimal(stringUUID[i+1]);
        }

        return uuid;
    }

    void printUUID() const 
    {
        iprintf("%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X", 
            bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5], bytes[6], bytes[7],
            bytes[8], bytes[9], bytes[10], bytes[11], 
            bytes[12], bytes[13], bytes[14], bytes[15]);
    }

private:
    uint8_t bytes[UUID_LEN];
};

} // namespace UUID

#define DEF_UUID(x) UUID::UUID::fromString(x)