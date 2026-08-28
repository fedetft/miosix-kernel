#pragma once
#include <string>
#include <cstring>
#include <format>

namespace UUID {
    constexpr size_t UUID_LEN = 16; // uuid is 16 bytes in length
    class UUID {
    public:
        constexpr UUID() = default;

        UUID(uint8_t bytes[UUID_LEN]) {
            std::memcpy(this->bytes, bytes, UUID_LEN);
        } 

        friend bool operator==(UUID& a, UUID& b) {
            return memcmp(a.bytes, b.bytes, UUID_LEN) == 0;
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

        void printUUID() {
            auto bytesPacked = reinterpret_cast<uint16_t*>(bytes);
            iprintf("%04X%04X-%04X-%04X-%04X-%04X%04X%04X", 
                bytesPacked[0], bytesPacked[1], bytesPacked[2], bytesPacked[3],
                bytesPacked[4], bytesPacked[5], bytesPacked[6], bytesPacked[7]);
        }

    private:
        uint8_t bytes[UUID_LEN];

        friend struct std::formatter<UUID>;
    };
}


/**
 * This formatter specialization is used to generate the UUID string equivalent 
 * of the provided uuid. It follows the RFC9562 (https://www.rfc-editor.org/info/rfc9562/) 
 * Section 4. UUID Format:
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
template <>
struct std::formatter<UUID::UUID> : std::formatter<std::string> {
    auto format(UUID::UUID uuid, format_context& ctx) const {
        auto bytes = uuid.bytes;
        return formatter<std::string>::format(
        std::format(
        "{:04x}{:04x}-{:04x}-{:04x}-{:04x}-{:04x}{:04x}{:04x}", 
            reinterpret_cast<uint16_t*>(bytes)[0],
            reinterpret_cast<uint16_t*>(bytes)[1],
            reinterpret_cast<uint16_t*>(bytes)[2],
            reinterpret_cast<uint16_t*>(bytes)[3],
            reinterpret_cast<uint16_t*>(bytes)[4],
            reinterpret_cast<uint16_t*>(bytes)[5],
            reinterpret_cast<uint16_t*>(bytes)[6],
            reinterpret_cast<uint16_t*>(bytes)[7]
        ), ctx);
    }
};

#define DEF_UUID(x) UUID::UUID::fromString(x)