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
#include "seed_case_tables.inc"
#include "seed_legacy_prefix.inc"
bool whitespace(uint32_t c) {
    return (c >= 9 && c <= 13) || c == 32 || c == 0x85 || c == 0xa0 ||
        c == 0x1680 || (c >= 0x2000 && c <= 0x200a) || c == 0x2028 ||
        c == 0x2029 || c == 0x202f || c == 0x205f || c == 0x3000;
}
bool utf8_next(const char*& p, uint32_t& cp) {
    const unsigned first = uint8_t(*p++);
    if (first < 128) { cp = first; return true; }
    unsigned remaining = 0, minimum = 0;
    if (first >= 0xc2 && first <= 0xdf) { cp = first & 31; remaining = 1; minimum = 0x80; }
    else if (first >= 0xe0 && first <= 0xef) { cp = first & 15; remaining = 2; minimum = 0x800; }
    else if (first >= 0xf0 && first <= 0xf4) { cp = first & 7; remaining = 3; minimum = 0x10000; }
    else return false;
    while (remaining--) {
        const unsigned next = uint8_t(*p);
        if ((next & 0xc0) != 0x80) return false;
        ++p; cp = (cp << 6) | (next & 63);
    }
    return cp >= minimum && cp <= 0x10ffff && !(cp >= 0xd800 && cp <= 0xdfff);
}
unsigned utf8_size(uint32_t cp) { return cp < 0x80 ? 1 : cp < 0x800 ? 2 : cp < 0x10000 ? 3 : 4; }
void utf8_write(char*& p, uint32_t cp) {
    if (cp < 0x80) *p++ = char(cp);
    else if (cp < 0x800) { *p++ = char(0xc0 | (cp >> 6)); *p++ = char(0x80 | (cp & 63)); }
    else if (cp < 0x10000) { *p++ = char(0xe0 | (cp >> 12)); *p++ = char(0x80 | ((cp >> 6) & 63)); *p++ = char(0x80 | (cp & 63)); }
    else { *p++ = char(0xf0 | (cp >> 18)); *p++ = char(0x80 | ((cp >> 12) & 63)); *p++ = char(0x80 | ((cp >> 6) & 63)); *p++ = char(0x80 | (cp & 63)); }
}
uint32_t seed_upper(uint32_t cp) {
    if (cp < 128) return cp >= 'a' && cp <= 'z' ? cp - ('a' - 'A') : cp;
    unsigned first = 0, end = sizeof SeedCaseRanges / sizeof *SeedCaseRanges;
    while (first < end) {
        const unsigned mid = first + (end - first) / 2;
        const auto& r = SeedCaseRanges[mid];
        if (cp < r.first) end = mid;
        else if (cp > (r.end & 0x7fffffffu)) first = mid + 1;
        else return r.end & 0x80000000u && ((cp - r.first) & 1) ? cp : uint32_t(int32_t(cp) + r.delta);
    }
    first = 0; end = sizeof SeedCasePairs / sizeof *SeedCasePairs;
    while (first < end) {
        const unsigned mid = first + (end - first) / 2;
        const auto& pair = SeedCasePairs[mid];
        if (cp < pair.lower) end = mid;
        else if (cp > pair.lower) first = mid + 1;
        else return pair.upper;
    }
    return cp;
}
unsigned seed_prefix_class(uint32_t cp) {
    unsigned first=0,end=sizeof SeedPrefixRanges/sizeof *SeedPrefixRanges;
    while(first<end){
        const unsigned mid=first+(end-first)/2;const auto&r=SeedPrefixRanges[mid];
        if(cp<r.first)end=mid;
        else if(cp>(r.end&0x7fffffffu))first=mid+1;
        else return r.end&0x80000000u?2:1;
    }
    return 0;
}
bool legacy_seed_prefix(const char*text){
    // String.StartsWith("old") in the source's invariant culture ignores
    // collation-zero characters but does not end before a combining accent.
    // The constructor still slices at UTF-16 index 3, not at the matched end.
    static constexpr char prefix[]="old";unsigned matched=0;
    while(*text){
        uint32_t cp;if(!utf8_next(text,cp))return false;
        const unsigned category=seed_prefix_class(cp);
        if(matched==3)return category!=2;
        if(category==1)continue;
        if(cp!=uint8_t(prefix[matched]))return false;
        ++matched;
    }
    return matched==3;
}
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

bool canonicalize_seed(const char* input, char* output, size_t capacity) {
    if (!output || !capacity) return false;
    output[0] = 0;
    if (!input) return false;
    const char* first = nullptr; const char* end = input;
    for (const char* p = input; *p;) {
        const char* begin = p; uint32_t cp;
        if (!utf8_next(p, cp)) return false;
        if (!whitespace(cp)) { if (!first) first = begin; end = p; }
    }
    if (!first) return true;
    char* dst = output;
    while (first < end) {
        uint32_t cp; utf8_next(first, cp); cp = seed_upper(cp);
        if (cp == 'O') cp = '0'; else if (cp == 'I') cp = '1';
        if (utf8_size(cp) >= capacity - size_t(dst - output)) { output[0] = 0; return false; }
        utf8_write(dst, cp);
    }
    *dst = 0; return true;
}

bool seed_hash(const char* input, uint64_t& value) {
    if (!input) return false;
    const bool legacy = legacy_seed_prefix(input);
    const char* p = input;
    uint32_t a = 352654597u, b = a; bool odd = false;unsigned skip=3;
    const auto unit = [&](uint16_t ch) {
        if(skip){--skip;return;}
        uint32_t& v = odd ? b : a; v = v * 33u ^ ch; odd = !odd;
    };
    while (*p) {
        uint32_t cp; if (!utf8_next(p, cp)) return false;
        if (legacy) {
            if (cp < 0x10000) unit(uint16_t(cp));
            else { cp -= 0x10000; unit(uint16_t(0xd800 + (cp >> 10))); unit(uint16_t(0xdc00 + (cp & 1023))); }
        }
    }
    value = legacy ? uint64_t(uint32_t(a + b * 1566083941u)) : hash64(input, size_t(p - input));
    return true;
}

bool seed_append(char* text, size_t capacity, uint32_t cp) {
    if (!text || !capacity || !cp || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) return false;
    size_t size = 0; while (size < capacity && text[size]) ++size;
    if (size == capacity || utf8_size(cp) >= capacity - size) return false;
    char* dst = text + size; utf8_write(dst, cp); *dst = 0; return true;
}

void seed_backspace(char* text) {
    if (!text) return;
    size_t size = 0; while (text[size]) ++size;
    if (!size) return;
    do { --size; } while (size && (uint8_t(text[size]) & 0xc0) == 0x80);
    text[size] = 0;
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
