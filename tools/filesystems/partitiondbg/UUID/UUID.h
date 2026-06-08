#pragma once
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <regex>

namespace UUID {
    constexpr size_t UUID_LEN = 16; // uuid is 16 bytes in length

    class UUID {
    public:
        constexpr UUID() = default;

        UUID(uint8_t bytes[UUID_LEN]) {
            std::memcpy(this->bytes, bytes, 16);
        } 

        friend bool operator==(UUID& a, UUID& b) {
            return memcmp(a.bytes, b.bytes, 16) == 0;
        }

        static std::string uint8_to_hex_string(const uint8_t *v, const size_t s) {
            std::stringstream ss;

            ss << std::hex << std::setfill('0');

            for (size_t i = 0; i < s; i++) {
                ss << std::hex << std::setw(2) << static_cast<int>(v[i]);
            }

            return ss.str();
        }
            
        /**
         * This method is used to generate the UUID string equivalent of the provided uuid.
         * It follows the RFC9562 (https://www.rfc-editor.org/info/rfc9562/) Section 4. UUID Format:
         * The formal definition of the UUID string representation is provided by the following ABNF [RFC5234]:
            ~~~
            UUID     = 4hexOctet "-"
                    2hexOctet "-"
                    2hexOctet "-"
                    2hexOctet "-"
                    6hexOctet
                    hexOctet = HEXDIG HEXDIG
            DIGIT    = %x30-39
            HEXDIG   = DIGIT / "A" / "B" / "C" / "D" / "E" / "F"
            ~~~   
         */
        friend std::ostream& operator<<(std::ostream& stream, UUID& uuid) {
            return stream << uint8_to_hex_string(&uuid.bytes[0], 4) << "-" 
                << uint8_to_hex_string(&uuid.bytes[4], 2) << "-"
                << uint8_to_hex_string(&uuid.bytes[6], 2) << "-"
                << uint8_to_hex_string(&uuid.bytes[8], 2) << "-"
                << uint8_to_hex_string(&uuid.bytes[10], 6) << "";
        }

        static consteval char toUpperAndDecimal(const char c) {
            if (c >= 'a' && c <= 'f') return c - 'a';
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'A' && c <= 'F') return c - 'A';
            throw "Invalid UUID";
        }

        static consteval void checkDash(const char c) {
            if (c == '-') return;

            throw "Invalid UUID";
        }

        static consteval UUID fromString(const char stringUUID[36]) {
            UUID uuid;
            auto bytes = uuid.bytes;
            size_t byteIdx = 0, i = 0;
            for (i = 0; i < 8; i+= 2, byteIdx++) {
                bytes[byteIdx] = toUpperAndDecimal(stringUUID[i]) << 4 | toUpperAndDecimal(stringUUID[i + 1]);
            }
            checkDash(stringUUID[i]);
            i++; // skip the dash
            
            for (; i < 13; i+= 2, byteIdx++) {
                bytes[byteIdx] = toUpperAndDecimal(stringUUID[i]) << 4 | toUpperAndDecimal(stringUUID[i + 1]);
            }

            checkDash(stringUUID[i]);
            i++; // skip the dash again

            for (; i < 18; i+= 2, byteIdx++) {
                bytes[byteIdx] = toUpperAndDecimal(stringUUID[i]) << 4 | toUpperAndDecimal(stringUUID[i + 1]);
            }

            checkDash(stringUUID[i]);
            i++; //skip the dash

            for(; i < 23; i+=2, byteIdx++) {
                bytes[byteIdx] = toUpperAndDecimal(stringUUID[i]) << 4 | toUpperAndDecimal(stringUUID[i + 1]);
            }

            checkDash(stringUUID[i]);
            i++; // skip the dash

            for(; i < 36; i+=2, byteIdx++) {
                bytes[byteIdx] = toUpperAndDecimal(stringUUID[i]) << 4 | toUpperAndDecimal(stringUUID[i + 1]);
            }
            return uuid;
        }
    private:
        uint8_t bytes[UUID_LEN];
    };
}

#define DEF_UUID(x) UUID::UUID::fromString(x)