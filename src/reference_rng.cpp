#include "reference_rng.hpp"

namespace spire::reference {
namespace {
constexpr uint64_t P1 = UINT64_C(11400714785074694791);
constexpr uint64_t P2 = UINT64_C(14029467366897019727);
constexpr uint64_t P3 = UINT64_C(1609587929392839161);
constexpr uint64_t P4 = UINT64_C(9650029242287828579);
constexpr uint64_t P5 = UINT64_C(2870177450012600261);
uint64_t rotate(uint64_t v, unsigned bits) { return (v << bits) | (v >> (64 - bits)); }
uint64_t read64(const uint8_t* p) {
    uint64_t v = 0; for (unsigned i = 0; i < 8; ++i) v |= uint64_t(p[i]) << (8 * i); return v;
}
uint32_t read32(const uint8_t* p) {
    uint32_t v = 0; for (unsigned i = 0; i < 4; ++i) v |= uint32_t(p[i]) << (8 * i); return v;
}
uint64_t round64(uint64_t a, uint64_t v) { return rotate(a + v * P2, 31) * P1; }
uint64_t merge(uint64_t a, uint64_t v) { return (a ^ round64(0, v)) * P1 + P4; }
uint64_t splitmix(uint64_t& value) {
    uint64_t z = (value += UINT64_C(0x9e3779b97f4a7c15));
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
}
bool whitespace(unsigned c) { return c == 32 || (c >= 9 && c <= 13); }
unsigned bit_length(uint64_t value) {
    unsigned n = 0; while (value) { ++n; value >>= 1; } return n;
}
}

uint64_t hash64(const void* bytes, size_t length, uint64_t seed) {
    const auto* p = static_cast<const uint8_t*>(bytes);
    size_t pos = 0; uint64_t h;
    if (length >= 32) {
        uint64_t a = seed + P1 + P2, b = seed + P2, c = seed, d = seed - P1;
        do {
            a = round64(a, read64(p + pos)); b = round64(b, read64(p + pos + 8));
            c = round64(c, read64(p + pos + 16)); d = round64(d, read64(p + pos + 24));
            pos += 32;
        } while (length - pos >= 32);
        h = rotate(a, 1) + rotate(b, 7) + rotate(c, 12) + rotate(d, 18);
        h = merge(merge(merge(merge(h, a), b), c), d);
    } else h = seed + P5;
    h += length;
    while (length - pos >= 8) {
        h ^= round64(0, read64(p + pos)); h = rotate(h, 27) * P1 + P4; pos += 8;
    }
    if (length - pos >= 4) { h ^= uint64_t(read32(p + pos)) * P1; h = rotate(h, 23) * P2 + P3; pos += 4; }
    while (pos < length) { h ^= uint64_t(p[pos++]) * P5; h = rotate(h, 11) * P1; }
    h ^= h >> 33; h *= P2; h ^= h >> 29; h *= P3; return h ^ (h >> 32);
}

bool canonicalize_ascii(const char* input, char* output, size_t capacity) {
    if (!output || !capacity) return false;
    output[0] = 0;
    if (!input) return false;
    size_t length = 0;
    while (input[length]) {
        unsigned c = static_cast<uint8_t>(input[length]);
        // ASCII input is a documented subset of ToUpperInvariant/Trim. Reject
        // unsupported Unicode rather than hashing a differently normalized seed.
        if (c >= 127 || (c < 32 && !whitespace(c))) return false;
        ++length;
    }
    size_t first = 0, end = length;
    while (first < end && whitespace(static_cast<uint8_t>(input[first]))) ++first;
    while (end > first && whitespace(static_cast<uint8_t>(input[end - 1]))) --end;
    if (end == first || end - first >= capacity) return false;
    for (size_t i = first; i < end; ++i) {
        char c = input[i]; if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
        if (c == 'O') c = '0'; else if (c == 'I') c = '1';
        output[i - first] = c;
    }
    output[end - first] = 0; return true;
}

uint32_t sample_bounded(uint64_t m, uint32_t bound) {
    // Exact 53-by-32-bit product in three 32-bit limbs.
    m &= (UINT64_C(1) << 53) - 1;
    uint64_t low_product = (m & UINT64_C(0xffffffff)) * bound;
    uint64_t high = (m >> 32) * bound + (low_product >> 32);
    uint32_t low = static_cast<uint32_t>(low_product);
    unsigned bits = high ? bit_length(high) + 32 : bit_length(low);
    if (bits > 53) {
        unsigned discarded = bits - 53; // At most 32.
        uint64_t unit = UINT64_C(1) << discarded;
        uint64_t remainder = low & (unit - 1), halfway = unit >> 1;
        bool odd = discarded == 32 ? (high & 1) : ((low >> discarded) & 1);
        if (remainder > halfway || (remainder == halfway && odd))
            high += (uint64_t(low) + unit) >> 32;
    }
    return static_cast<uint32_t>(high >> 21);
}

void Random::reset(uint64_t seed) {
    for (auto& word : words) word = splitmix(seed);
    calls = 0; good = true;
}
void Random::named(uint64_t run_seed, const char* name) {
    if (!name) { good = false; return; }
    size_t n = 0; while (name[n]) ++n;
    reset(run_seed + hash64(name, n));
}
uint64_t Random::next() {
    if (!good) return 0;
    uint64_t result = rotate(words[1] * 5, 7) * 9, shifted = words[1] << 17;
    words[2] ^= words[0]; words[3] ^= words[1]; words[1] ^= words[2]; words[0] ^= words[3];
    words[2] ^= shifted; words[3] = rotate(words[3], 45); ++calls; return result;
}
uint32_t Random::below(uint32_t bound) {
    if (!bound) { good = false; return 0; }
    return sample_bounded(next() >> 11, bound);
}
int32_t Random::range(int32_t low, int32_t high) {
    if (low >= high) { good = false; return low; }
    uint32_t width = static_cast<uint32_t>(int64_t(high) - low);
    return static_cast<int32_t>(int64_t(low) + below(width));
}
} // namespace spire::reference
