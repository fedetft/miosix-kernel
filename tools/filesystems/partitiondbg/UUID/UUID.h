#pragma once
#include <string>
#include <iostream>
#include <cstring>

namespace UUID {
    constexpr size_t UUID_LEN = 16; // uuid is 16 bytes in length
    
    class UUID {
    public:
        UUID(uint8_t bytes[UUID_LEN]) {
            std::memcpy(this->bytes, bytes, 16);
        } 

        friend bool operator==(UUID& a, UUID& b) {
            return memcmp(a.bytes, b.bytes, 16) == 0;
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
            return stream << std::hex << uuid.bytes[0] << uuid.bytes[1] << uuid.bytes[2] << uuid.bytes[3] << "-" 
                << std::hex << uuid.bytes[4] << uuid.bytes[5] << "-"
                << std::hex << uuid.bytes[6] << uuid.bytes[7] << "-"
                << std::hex << uuid.bytes[8] << uuid.bytes[9] << "-"
                << std::hex << uuid.bytes[10] << uuid.bytes[11] << uuid.bytes[12] << uuid.bytes[13] << uuid.bytes[14] << uuid.bytes[15];
        }
    private:
        uint8_t bytes[UUID_LEN];
    };
}