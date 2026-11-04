#pragma once
#include <stdint.h>
namespace spire::ancient_material_residual {
constexpr unsigned First = 59681, Count = 14, PageWidth = 320, PageHeight = 240;
constexpr unsigned Reconstructed = 8;
constexpr bool page(unsigned id) {
    return id >= First && id < First + Count;
}
// An identical residual has a different meaning at each source coordinate.
// Only these typed leaves bypass ordinary immutable-payload alias sharing.
constexpr bool same_identity(unsigned a, unsigned b, bool static_profile = false) {
    return a == b || (!page(a) && !page(b)) || static_profile;
}
inline unsigned mix(unsigned a, unsigned b, unsigned f) {
    return ((a << 8) + (b - a) * f + 128) >> 8;
}
inline uint16_t mix_color(uint16_t a, uint16_t b, unsigned f) {
    return uint16_t((mix(a >> 11, b >> 11, f) << 11) | (mix((a >> 5) & 63, (b >> 5) & 63, f) << 5) |
                    mix(a & 31, b & 31, f));
}
inline uint16_t add(uint16_t predicted, uint16_t residual) {
    return uint16_t(((((predicted >> 11) + (residual >> 11)) & 31) << 11) |
                    (((((predicted >> 5) & 63) + ((residual >> 5) & 63)) & 63) << 5) |
                    ((predicted + residual) & 31));
}
template <class Texture> inline uint16_t base_color(const Texture& base, int x, int y) {
    const auto value = base.at(x, y);
    return value.alpha ? value.color : 0;
}
template <class Texture>
bool restore_response(uint8_t* bytes, const Texture& left, const Texture& right) {
    if (!bytes || !left.valid() || !right.valid() || left.width() != 320 || left.height() != 240 ||
        right.width() != 320 || right.height() != 240)
        return false;
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x) {
            const unsigned at = (y * 320 + x) * 2;
            const uint16_t a = base_color(left, int(x), int(y)),
                           b = base_color(right, int(x), int(y));
            const uint16_t pred = uint16_t(((((a >> 11) + (b >> 11) + 1) >> 1) << 11) |
                                           (((((a >> 5) & 63) + ((b >> 5) & 63) + 1) >> 1) << 5) |
                                           (((a & 31) + (b & 31) + 1) >> 1));
            const uint16_t result = add(pred, uint16_t(bytes[at] | unsigned(bytes[at + 1]) << 8));
            bytes[at] = uint8_t(result);
            bytes[at + 1] = uint8_t(result >> 8);
        }
    return true;
}

struct Mapping {
    int32_t x = 0, y = 0, dx = 0, dy = 0;
    bool valid() const {
        return x >= -16 * 65536 && x <= 16 * 65536 && y >= -16 * 65536 && y <= 16 * 65536 &&
               dx > 0 && dx < 65536 && dy > 0 && dy < 65536 &&
               int64_t(x) + 959 * int64_t(dx) < 336 * 65536 &&
               int64_t(y) + 719 * int64_t(dy) < 256 * 65536;
    }
    template <class Texture> uint16_t predict(const Texture& base, unsigned px, unsigned py) const {
        const int xx = x + int(px) * dx, yy = y + int(py) * dy;
        const int left = xx >> 16, top = yy >> 16;
        const unsigned fx = unsigned(xx & 65535) >> 8, fy = unsigned(yy & 65535) >> 8;
        return mix_color(
            mix_color(base_color(base, left, top), base_color(base, left + 1, top), fx),
            mix_color(base_color(base, left, top + 1), base_color(base, left + 1, top + 1), fx),
            fy);
    }
    template <class Texture> bool restore(uint8_t* bytes, unsigned id, const Texture& base) const {
        if (!bytes || !page(id) || !valid() || !base.valid() || base.width() != 320 ||
            base.height() != 240)
            return false;
        const unsigned index = (id - First) % 9, origin_x = (index % 3) * 320,
                       origin_y = (index / 3) * 240;
        for (unsigned row = 0; row < 240; ++row)
            for (unsigned col = 0; col < 320; ++col) {
                const unsigned at = (row * 320 + col) * 2;
                const unsigned residual = bytes[at] | unsigned(bytes[at + 1]) << 8;
                const unsigned predicted = predict(base, origin_x + col, origin_y + row);
                const unsigned result =
                    ((((predicted >> 11) + (residual >> 11)) & 31) << 11) |
                    (((((predicted >> 5) & 63) + ((residual >> 5) & 63)) & 63) << 5) |
                    ((predicted + residual) & 31);
                bytes[at] = uint8_t(result);
                bytes[at + 1] = uint8_t(result >> 8);
            }
        return true;
    }
};
} // namespace spire::ancient_material_residual
