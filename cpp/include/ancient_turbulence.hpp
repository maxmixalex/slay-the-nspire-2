#pragma once
#include <stdint.h>
namespace spire::ancient_turbulence {
constexpr unsigned AuxiliaryFirst = 7223, AuxiliaryCount = 7224, FirstImage = 59717, Images = 59729;
constexpr unsigned EmitterCount = 12, HeaderBytes = 64, RampBytes = 2048, DirectoryBytes = 12;
inline bool view_geometry(unsigned node, unsigned width, unsigned height) {
    return node < EmitterCount && (node == 5 ? (width == 320 && height == 205)
                                             : (width == 256 && height > 0 && height <= 240));
}
inline unsigned word(const uint8_t* p) {
    return p[0] | unsigned(p[1]) << 8;
}
inline uint32_t dword(const uint8_t* p) {
    return word(p) | uint32_t(word(p + 2)) << 16;
}
inline int signed_word(const uint8_t* p) {
    return int16_t(word(p));
}
inline uint32_t hash(uint32_t v) {
    v ^= v >> 16;
    v *= 0x7feb352du;
    v ^= v >> 15;
    v *= 0x846ca68bu;
    return v ^ (v >> 16);
}
struct Sample {
    int x = 0, y = 0, angle = 0;
    unsigned scale = 0, phase = 0;
};
struct Table {
    const uint8_t* data = nullptr;
    unsigned bytes = 0, count = 0, life = 0, preprocess = 0, fps = 0, blend = 0;
    unsigned width = 0, height = 0, texture = 0, ramps = 0, directory = 0, palette = 0,
             extent_x = 0, extent_y = 0;
    bool range(unsigned offset, unsigned size) const {
        return offset <= bytes && size <= bytes - offset;
    }
    bool read(const uint8_t* p, unsigned size) {
        data = nullptr;
        if (!p || size < HeaderBytes || p[0] != 'A' || p[1] != 'T' || p[2] != 'P' || p[3] != '1' ||
            word(p + 4) != 2)
            return false;
        bytes = dword(p + 36);
        if (bytes > size || bytes < HeaderBytes || dword(p + 60))
            return false;
        count = word(p + 6);
        life = dword(p + 8);
        preprocess = dword(p + 12);
        fps = word(p + 16);
        blend = word(p + 18);
        width = word(p + 20);
        height = word(p + 22);
        texture = dword(p + 24);
        ramps = dword(p + 28);
        directory = dword(p + 32);
        extent_x = dword(p + 40);
        extent_y = dword(p + 44);
        palette = dword(p + 56);
        if (!count || count > 100 || !life || life > 30000 || preprocess > 100000 || fps != 30 ||
            blend > 1 || !width || width > 320 || !height || height > 256 || !extent_x ||
            !extent_y || extent_x > 65535 || extent_y > 65535 || !range(texture, width * height) ||
            !range(ramps, RampBytes) || !range(palette, 512) ||
            !range(directory, count * DirectoryBytes))
            return false;
        data = p;
        return true;
    }
    unsigned tint(unsigned channel) const {
        return word(data + 48 + channel * 2);
    }
    // Six byte planes retain the exact same quantized keys. Their slowly varying
    // high bytes compress better without a predictor, extra buffer or warm work.
    static unsigned key(const uint8_t* p, unsigned count, unsigned index, unsigned channel) {
        p += channel * 2 * count + index;
        return p[0] | unsigned(p[count]) << 8;
    }
    // Source normalized texture-center interpolation; ramps retain color bytes,
    // alpha16 and scaleQ12 separately. No per-frame resource-wide validation.
    unsigned ramp(unsigned channel, unsigned phase) const {
        const int x = int(phase * 256) - 32768, i = x >> 16;
        const unsigned f = unsigned(x & 65535) >> 8;
        const unsigned a = unsigned(i < 0     ? 0
                                    : i > 255 ? 255
                                              : i),
                       b = unsigned(i + 1 < 0     ? 0
                                    : i + 1 > 255 ? 255
                                                  : i + 1);
        unsigned av, bv;
        if (channel < 4) {
            av = data[ramps + a * 4 + channel];
            bv = data[ramps + b * 4 + channel];
        } else {
            const unsigned base = ramps + 1024 + (channel - 4) * 512;
            av = word(data + base + a * 2);
            bv = word(data + base + b * 2);
        }
        return (av * (256 - f) + bv * f + 128) >> 8;
    }
    bool sample(unsigned particle, uint32_t elapsed, uint32_t seed, Sample& out) const {
        if (!data || particle >= count)
            return false;
        // Each birth selects a deterministic recorded source seed. The clock and
        // particle capacity are original; this does not claim a captured GPU RNG.
        const uint64_t t =
            uint64_t(elapsed) + preprocess + life - uint64_t(particle) * life / count;
        const unsigned age = unsigned(t % life), cycle = unsigned(t / life);
        const unsigned track = (particle + hash(seed ^ cycle * 0x9e3779b9u) % count) % count;
        const uint8_t* record = data + directory + track * DirectoryBytes;
        const unsigned offset = dword(record), keys = word(record + 4), ticks = life * fps / 1000;
        if (keys < 2 || keys > ticks + 1 || !range(offset, keys * 6) || word(record + 10))
            return false;
        const uint8_t* points = data + offset;
        if (key(points, keys, 0, 0) || key(points, keys, keys - 1, 0) != ticks)
            return false;
        const unsigned time = unsigned(uint64_t(age) * fps * 256 / 1000);
        unsigned lo = 0, hi = keys - 1;
        while (hi - lo > 1) {
            const unsigned middle = (lo + hi) / 2;
            if (key(points, keys, middle, 0) * 256 <= time)
                lo = middle;
            else
                hi = middle;
        }
        const unsigned start = key(points, keys, lo, 0) * 256, end = key(points, keys, hi, 0) * 256;
        if (end <= start || time < start || time > end)
            return false;
        const unsigned passed = time - start, span = end - start;
        // The Q4 stored keys are interpolated into Q8 LCD coordinates. Offline
        // validation includes a sqrt(2)/256px bound for this signed truncation.
        const int ax = int16_t(key(points, keys, lo, 1)), ay = int16_t(key(points, keys, lo, 2));
        out.x = ax * 16 + int(int64_t(int16_t(key(points, keys, hi, 1)) - ax) * 16 * passed / span);
        out.y = ay * 16 + int(int64_t(int16_t(key(points, keys, hi, 2)) - ay) * 16 * passed / span);
        out.scale = word(record + 6);
        out.phase = unsigned(uint64_t(age) * 65536 / life);
        out.angle = -int(int64_t(signed_word(record + 8)) * age * 4096 / (128LL * 360000));
        return true;
    }
};
} // namespace spire::ancient_turbulence
