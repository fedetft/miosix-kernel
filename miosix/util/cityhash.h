#pragma once

#include <cstddef>
#include <cstdint>

namespace miosix {

/**
 * Calculate a reduced version of Google's CityHash on a short string of bytes.
 * Only works for strings of length 5 to 12 bytes!
 * \param s string of bytes
 * \param len string length
 * \return a 32-bit hash of the input string
 */
uint32_t cityHash32(const uint8_t *s, size_t len);

} // namespace miosix