#pragma once
#include <stddef.h>
#include <stdint.h>

namespace spire::reference {
// Version-pinned seed primitives. This is deliberately separate from legacy
// expedition saves; swapping their generator would silently change existing runs.
uint64_t hash64(const void* bytes, size_t length, uint64_t seed = 0);
bool canonicalize_ascii(const char* input, char* output, size_t capacity);

// floor(binary64(mantissa * bound) / 2^53), including nearest-even rounding.
// No floating-point runtime or 128-bit integer support is required on ARMv5.
uint32_t sample_bounded(uint64_t mantissa53, uint32_t bound);
struct Random {
    uint64_t words[4]{};
    uint32_t calls = 0;
    bool good = false;
    void reset(uint64_t seed);
    void named(uint64_t run_seed, const char* stream_name);
    uint64_t next();
    uint32_t below(uint32_t exclusive_max);
    int32_t range(int32_t inclusive_min, int32_t exclusive_max);
};

template<class T> void shuffle(T* values, unsigned count, Random& rng) {
    // Audited ListExtensions.UnstableShuffle: n-1 draws, no draw for 0/1 items.
    for (unsigned n = count; n > 1; --n) {
        unsigned i = rng.below(n);
        T value = values[n - 1]; values[n - 1] = values[i]; values[i] = value;
    }
}
} // namespace spire::reference
