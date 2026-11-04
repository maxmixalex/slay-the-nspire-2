#pragma once
#include <stdint.h>
#include "fake_merchant_presentation.hpp"
namespace spire::neow_particles {
constexpr unsigned Auxiliary = 7226, AuxiliaryCount = 7227, FirstImage = 59731, Images = 59732,
                   Bytes = 4608, AtlasFirst = 59712;
inline unsigned word(const uint8_t* p) {
    return p[0] | unsigned(p[1]) << 8;
}
inline unsigned dword(const uint8_t* p) {
    return word(p) | uint32_t(word(p + 2)) << 16;
}
struct Parameters {
    const uint8_t* data = nullptr;
    template <class Picture> bool read(Picture p) {
        data = nullptr;
        if (!p.pixels || p.w != 256 || p.h != 9 || p.format || p.alpha || p.palette)
            return false;
        const auto* b = p.pixels;
        if (b[0] != 'N' || b[1] != 'N' || b[2] != 'P' || b[3] != '1' || word(b + 4) != 1 ||
            word(b + 6) != AtlasFirst || word(b + 8) != 4 || word(b + 10) != 32 ||
            word(b + 12) != 32 || word(b + 14) != 3 || dword(b + 16) != 64 ||
            dword(b + 20) != 1088 || dword(b + 24) != 2112 || dword(b + 28) != 2624 ||
            dword(b + 32) != 2880 || dword(b + 36) != 2910 || dword(b + 40) != 3422 ||
            dword(b + 44) != 4446)
            return false;
        for (unsigned i = 56; i < 64; ++i)
            if (b[i])
                return false;
        if (word(b + 2880) || word(b + 2900) != 65535 || !word(b + 2890) || word(b + 2890) >= 65535)
            return false;
        data = b;
        return true;
    }
    int sample(unsigned offset, unsigned width, unsigned phase) const {
        const int at = int(phase * 256) - 32768, index = at >> 16;
        const unsigned fraction = unsigned(at) & 65535u;
        const unsigned low = index < 0     ? 0
                             : index > 255 ? 255
                                           : unsigned(index),
                       high = index + 1 < 0     ? 0
                              : index + 1 > 255 ? 255
                                                : unsigned(index + 1);
        const auto value = [&](unsigned i) {
            const auto* p = data + offset + i * width;
            return width == 4 ? int32_t(dword(p)) : width == 2 ? int(word(p)) : int(*p);
        };
        const int a = value(low), b = value(high);
        return a + int(int64_t(b - a) * fraction / 65536);
    }
    unsigned star_color(unsigned channel, unsigned phase) const {
        if (phase > 65535)
            phase = 65535;
        const auto* p = data + 2880;
        unsigned right = phase <= word(p + 10) ? 1 : 2;
        const auto *a = p + (right - 1) * 10, *b = p + right * 10;
        const unsigned start = word(a), end = word(b);
        const int first = int(word(a + 2 + channel * 2)), last = int(word(b + 2 + channel * 2));
        return unsigned(first + int(int64_t(last - first) * (phase - start) / (end - start)));
    }
};
struct Fog {
    int x, scale, angle;
    unsigned atlas, gray;
};
inline Fog fog(const Parameters& p, uint32_t elapsed, uint32_t seed, unsigned node,
               unsigned index) {
    using namespace fake_merchant;
    const uint64_t t = uint64_t(elapsed) + 100000 - index * 2000;
    const unsigned age = unsigned(t % 10000), cycle = unsigned(t / 10000),
                   phase = age * 65536u / 10000;
    const uint32_t random =
        hash(seed ^ node * 0x6c8e9cf5u ^ index * 0x9e3779b9u ^ cycle * 0x85ebca6bu);
    const unsigned tick = age * 30 / 1000, fraction = age * 30 % 1000;
    // Source fixed_fps0 preprocessing uses30Hz. Interpolate the exact
    // semi-implicit gravity sum; no particle array or prewarm loop is needed.
    const int drift = int((uint64_t(50) * tick * (tick + 1) * 256 * 1000 +
                           uint64_t(100) * (tick + 1) * fraction * 256) /
                          1800000);
    const int initial = 4 * Q + int(hash(random) % unsigned(2 * Q + 1));
    return {node == 2 ? -drift : drift, initial * p.sample(2112, 2, phase) / Q,
            p.sample(node == 2 ? 1088 : 64, 4, phase) / 256, hash(random + 1) & 3u,
            unsigned(p.sample(2624, 1, phase))};
}
struct Star {
    int x, y, scale;
    unsigned phase;
};
inline Star star(uint32_t elapsed, uint32_t seed, unsigned index) {
    using namespace fake_merchant;
    const uint64_t t = uint64_t(elapsed) + 5000 - index * 500;
    const unsigned age = unsigned(t % 5000), cycle = unsigned(t / 5000);
    const uint32_t random = hash(seed ^ index * 0x9e3779b9u ^ cycle * 0x85ebca6bu);
    const int initial_x = int(int64_t(hash(random) & 65535u) * 3000 * 256 / 65535) - 1500 * 256;
    const int initial_y = int(int64_t(hash(random + 1) & 65535u) * 1600 * 256 / 65535) - 800 * 256;
    const unsigned angle = hash(random + 2) & 4095u, tick = age * 30 / 1000,
                   fraction = age * 30 % 1000;
    const int gravity =
        int((uint64_t(tick) * (tick + 1) * 256 * 1000 + uint64_t(2) * (tick + 1) * fraction * 256) /
            1800000);
    const int x =
        initial_x +
        int(int64_t(CombatCardMotion::sine(int(angle) + 1024)) * 50 * age * 256 / 1024000) -
        gravity;
    const int y =
        initial_y + int(int64_t(CombatCardMotion::sine(int(angle))) * 50 * age * 256 / 1024000);
    // Retain the authored.999997 node scale at Q30 precision.
    return {1003 * 256 + int(int64_t(x) * 1073738603 / 1073741824),
            590 * 256 + int(int64_t(y) * 1073738603 / 1073741824),
            Q + int(hash(random + 3) % unsigned(Q + 1)), age * 65536u / 5000};
}
} // namespace spire::neow_particles
