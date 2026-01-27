#include <cstdio>
#include <cstring>

namespace {

constexpr uint32_t fetch32(const uint8_t *p) {
    // Unaligned load of a 32-bit value
    uint32_t result;
    std::memcpy(&result, p, sizeof(result));
    return result;
}

constexpr uint32_t rotate32(uint32_t val, int shift) {
    // Avoid shifting by 32, doing so yields an undefined result
    return shift == 0 ? val : ((val >> shift) | (val << (32 - shift)));
}

// A 32-bit to 32-bit integer hash copied from Murmur3
constexpr uint32_t fmix(uint32_t h) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

// Multiply and rotate
constexpr uint32_t mur(uint32_t a, uint32_t h) {
    // Magic numbers for 32-bit hashing, copied from Murmur3
    constexpr uint32_t c1 = 0xcc9e2d51;
    constexpr uint32_t c2 = 0x1b873593;

    // Helper from Murmur3 for combining two 32-bit values
    a *= c1;
    a = rotate32(a, 17);
    a *= c2;
    h ^= a;
    h = rotate32(h, 19);
    return h * 5 + 0xe6546b64;
}

} // namespace

namespace miosix {

uint32_t cityHash32(const uint8_t *s, size_t len) {
    uint32_t a = static_cast<uint32_t>(len);
    uint32_t b = a * 5;
    uint32_t c = 9;
    uint32_t d = b;

    a += fetch32(s);
    b += fetch32(s + len - 4);
    c += fetch32(s + ((len >> 1) & 4));

    return fmix(mur(c, mur(b, mur(a, d))));
}

} // namespace miosix
