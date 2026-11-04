#pragma once
#include <stdint.h>
#include "tanx_scene.hpp"

namespace spire::ancient_lights {
constexpr unsigned AuxiliaryFirst = tanx_scene::AuxiliaryCount;
constexpr unsigned AuxiliaryCount = AuxiliaryFirst + 2;
constexpr unsigned FirstImage = tanx_scene::Images, Images = FirstImage + 3;
inline unsigned word(const uint8_t* p) {
    return unsigned(p[0]) | unsigned(p[1]) << 8;
}
inline int signed_word(const uint8_t* p) {
    const unsigned n = word(p);
    return n < 32768 ? int(n) : int(n) - 65536;
}
inline int32_t integer(const uint8_t* p) {
    const uint32_t n = word(p) | (uint32_t(word(p + 2)) << 16);
    return n <= 0x7fffffffu ? int32_t(n) : -1 - int32_t(~n);
}
struct Table {
    const uint8_t* data = nullptr;
    unsigned nodes = 0, frames = 0, duration = 0, dynamic = 0;
    template <class Picture> bool read(Picture p, unsigned scene) {
        data = nullptr;
        if (scene > 1 || !p.pixels || p.w != 256 || p.format || p.alpha || p.palette)
            return false;
        nodes = scene ? 90 : 10;
        dynamic = scene ? 17 : 3;
        frames = scene ? 506 : 546;
        duration = scene ? 16667 : 18000;
        const unsigned length = 32 + nodes * 32 + dynamic * frames * 12;
        if (p.h != (length + 511) / 512)
            return false;
        const auto* d = p.pixels;
        const uint16_t expected[] = {0x4c41,
                                     1,
                                     uint16_t(scene),
                                     uint16_t(nodes),
                                     uint16_t(dynamic),
                                     uint16_t(frames),
                                     33,
                                     uint16_t(duration),
                                     0,
                                     32,
                                     32,
                                     uint16_t(32 + nodes * 32),
                                     uint16_t(length),
                                     uint16_t(length >> 16),
                                     uint16_t(scene ? 65535 : FirstImage),
                                     0};
        for (unsigned i = 0; i < 16; ++i)
            if (word(d + i * 2) != expected[i])
                return false;
        unsigned moving = 0, particles = 0, alternate = 0;
        for (unsigned i = 0; i < nodes; ++i) {
            const auto* r = d + 32 + i * 32;
            for (unsigned j = 0; j < 6; ++j)
                if (integer(r + j * 4) <= -8388608 || integer(r + j * 4) >= 8388608)
                    return false;
            if (integer(r + 8) <= 0 || integer(r + 12) <= 0 || !r[28] || r[28] > 2 ||
                r[30] != (scene || i >= 7 ? 1 : 0) || r[31] > 1)
                return false;
            if (r[29] != 255 && r[29] != moving++)
                return false;
            particles += r[28];
            alternate += r[31];
        }
        if (moving != dynamic || particles != (scene ? 179u : 20u) || alternate != scene)
            return false;
        data = d;
        return true;
    }
    const uint8_t* node(unsigned index) const {
        return data + 32 + index * 32;
    }
    int coordinate(const uint8_t* r, unsigned component, unsigned frame) const {
        int value = integer(r + component * 4);
        if (r[29] != 255)
            value += signed_word(data + 32 + nodes * 32 + (unsigned(r[29]) * frames + frame) * 12 +
                                 component * 2);
        return value;
    }
    unsigned frame(uint32_t clock, bool reduced) const {
        return reduced ? 0 : (clock % duration) / 33;
    }
};
} // namespace spire::ancient_lights
