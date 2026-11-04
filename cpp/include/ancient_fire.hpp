#pragma once
#include <stdint.h>
#include "ancient_fire_noise.hpp"
namespace spire::ancient_fire {
constexpr unsigned Auxiliary = 7227, AuxiliaryCount = 7228, FirstImage = 59732, PageCount = 8,
                   ParameterImage = 59740, Images = 59741;
constexpr unsigned PageBytes = 131072, Nodes = 237, Frames = 506, Dynamic = 21, Models = 3,
                   Commands = 401, NodeBytes = 40;
constexpr unsigned NodesAt = 64, ModelsAt = NodesAt + Nodes * NodeBytes,
                   DeltasAt = ModelsAt + Models * 128, OrderAt = DeltasAt + Dynamic * Frames * 12,
                   Length = OrderAt + Commands * 2;
inline unsigned word(const uint8_t* p) {
    return p[0] | unsigned(p[1]) << 8;
}
inline uint32_t dword(const uint8_t* p) {
    return word(p) | uint32_t(word(p + 2)) << 16;
}
inline int integer(const uint8_t* p) {
    return int32_t(dword(p));
}
inline int bound(int x, int a, int b) {
    return x < a ? a : x > b ? b : x;
}
inline int product(int a, int b) {
    return int((int64_t(a) * b) >> 20);
}
inline int scaled(int a, int q24) {
    return int((int64_t(a) * q24) >> 24);
}
struct Field {
    const uint8_t* pages[8]{};
    unsigned width = 0, height = 0, channels = 0;
    struct Coordinates {
        unsigned at[4], fx, fy;
    };
    unsigned texel(unsigned at) const {
        return pages[at >> 17][at & (PageBytes - 1)];
    }
    __attribute__((always_inline)) Coordinates coordinates(int u, int v, bool repeat) const {
        if (repeat) {
            u = int(unsigned(u) & 1048575u);
            v = int(unsigned(v) & 1048575u);
        } else {
            u = bound(u, 0, 1048576);
            v = bound(v, 0, 1048576);
        }
        const int px = u * int(width) - 524288, py = v * int(height) - 524288;
        const int x = px >> 20, y = py >> 20;
        unsigned x0, x1, y0, y1;
        if (repeat) {
            x0 = unsigned(x < 0 ? x + int(width) : x);
            x1 = x0 + 1 == width ? 0 : x0 + 1;
            y0 = unsigned(y < 0 ? y + int(height) : y);
            y1 = y0 + 1 == height ? 0 : y0 + 1;
        } else {
            x0 = unsigned(bound(x, 0, int(width) - 1));
            x1 = unsigned(bound(x + 1, 0, int(width) - 1));
            y0 = unsigned(bound(y, 0, int(height) - 1));
            y1 = unsigned(bound(y + 1, 0, int(height) - 1));
        }
        const unsigned row0 = y0 * width, row1 = y1 * width;
        return {{(row0 + x0) * channels, (row0 + x1) * channels, (row1 + x0) * channels,
                 (row1 + x1) * channels},
                unsigned(px) & 1048575u,
                unsigned(py) & 1048575u};
    }
    // Pinned field dimensions make the source addressing constants visible to
    // ARM code generation. Sampling/rounding remains identical to coordinates().
    template <unsigned W, unsigned H, unsigned Channels, bool Repeat>
    __attribute__((always_inline)) Coordinates fixed_coordinates(int u, int v) const {
        if (Repeat) {
            u = int(unsigned(u) & 1048575u);
            v = int(unsigned(v) & 1048575u);
        } else {
            u = bound(u, 0, 1048576);
            v = bound(v, 0, 1048576);
        }
        const int px = u * int(W) - 524288, py = v * int(H) - 524288, x = px >> 20, y = py >> 20;
        unsigned x0, x1, y0, y1;
        if (Repeat) {
            x0 = unsigned(x < 0 ? x + int(W) : x);
            x1 = x0 + 1 == W ? 0 : x0 + 1;
            y0 = unsigned(y < 0 ? y + int(H) : y);
            y1 = y0 + 1 == H ? 0 : y0 + 1;
        } else {
            x0 = unsigned(bound(x, 0, int(W) - 1));
            x1 = unsigned(bound(x + 1, 0, int(W) - 1));
            y0 = unsigned(bound(y, 0, int(H) - 1));
            y1 = unsigned(bound(y + 1, 0, int(H) - 1));
        }
        return {{(y0 * W + x0) * Channels, (y0 * W + x1) * Channels, (y1 * W + x0) * Channels,
                 (y1 * W + x1) * Channels},
                unsigned(px) & 1048575u,
                unsigned(py) & 1048575u};
    }
    template <unsigned W, unsigned H, unsigned Channels, unsigned Channel, bool Repeat>
    int fixed_lcd(int u, int v) const {
        return sample_lcd(fixed_coordinates<W, H, Channels, Repeat>(u, v), Channel);
    }
    template <unsigned W, unsigned H, unsigned Channels, bool Repeat>
    void fixed_lcd_rg(int u, int v, int& r, int& g) const {
        const auto c = fixed_coordinates<W, H, Channels, Repeat>(u, v);
        if constexpr (Channels == 2 || (Channels == 3 && W * H * Channels <= 2 * PageBytes)) {
            // RGB starts are multiples of3. The sole page boundary is2 mod3,
            // so its R/G pair cannot straddle it; two-channel pairs are aligned.
            // Share each corner's page lookup, retaining byte loads on ARM926.
            unsigned colors[4];
            for (unsigned i = 0; i < 4; ++i) {
                const unsigned at = c.at[i];
                const auto* p = pages[at >> 17] + (at & (PageBytes - 1));
                colors[i] = unsigned(p[0]) | unsigned(p[1]) << 8;
            }
            const unsigned fx = c.fx >> 8, fy = c.fy >> 8;
            const auto channel = [&](unsigned shift) {
                const int p00 = int((colors[0] >> shift) & 255u),
                          p10 = int((colors[1] >> shift) & 255u),
                          p01 = int((colors[2] >> shift) & 255u),
                          p11 = int((colors[3] >> shift) & 255u);
                const int a = p00 * 4096 + (p10 - p00) * int(fx),
                          b = p01 * 4096 + (p11 - p01) * int(fx);
                const uint32_t weighted = (uint32_t(a) << 12) + uint32_t(b - a) * fy;
                return int((weighted + 2040u) / 4080u);
            };
            r = channel(0);
            g = channel(8);
        } else {
            r = sample_lcd(c, 0);
            g = sample_lcd(c, 1);
        }
    }
    int sample(const Coordinates& c, unsigned channel) const {
        const int p00 = int(texel(c.at[0] + channel)), p10 = int(texel(c.at[1] + channel));
        const int p01 = int(texel(c.at[2] + channel)), p11 = int(texel(c.at[3] + channel));
        // Both horizontal values are in [0,255*Q20]. A signed difference
        // retains the old round-to-nearest vertical result with one wide product.
        const int a = p00 * 1048576 + (p10 - p00) * int(c.fx);
        const int b = p01 * 1048576 + (p11 - p01) * int(c.fx);
        const unsigned q = unsigned(a + ((int64_t(b - a) * c.fy + 524288) >> 20));
        return int((uint64_t(q) * 16843009u + 2147483648u) >> 32);
    }
    unsigned texel(unsigned x, unsigned y, unsigned channel) const {
        const unsigned at = (y * width + x) * channels + channel;
        return pages[at >> 17][at & (PageBytes - 1)];
    }
    // Bilinear normalized sampling, including the original half-texel phase.
    // Returns Q20. Stored8-bit sampled pixels are not re-quantized here.
    int sample(int u, int v, unsigned channel, bool repeat) const {
        if (repeat) {
            u = int(unsigned(u) & 1048575u);
            v = int(unsigned(v) & 1048575u);
        } else {
            u = bound(u, 0, 1048576);
            v = bound(v, 0, 1048576);
        }
        const int px = u * int(width) - 524288, py = v * int(height) - 524288;
        int x = px >> 20, y = py >> 20;
        const unsigned fx = unsigned(px) & 1048575u, fy = unsigned(py) & 1048575u;
        unsigned x0, x1, y0, y1;
        if (repeat) {
            x0 = unsigned(x < 0 ? x + int(width) : x);
            x1 = x0 + 1 == width ? 0 : x0 + 1;
            y0 = unsigned(y < 0 ? y + int(height) : y);
            y1 = y0 + 1 == height ? 0 : y0 + 1;
        } else {
            x0 = unsigned(bound(x, 0, int(width) - 1));
            x1 = unsigned(bound(x + 1, 0, int(width) - 1));
            y0 = unsigned(bound(y, 0, int(height) - 1));
            y1 = unsigned(bound(y + 1, 0, int(height) - 1));
        }
        const unsigned a = texel(x0, y0, channel) * (1048576 - fx) + texel(x1, y0, channel) * fx;
        const unsigned b = texel(x0, y1, channel) * (1048576 - fx) + texel(x1, y1, channel) * fx;
        const unsigned q =
            unsigned((uint64_t(a) * (1048576 - fy) + uint64_t(b) * fy + 524288) >> 20);
        return int((uint64_t(q) * 16843009u + 2147483648u) >> 32); // Exact byte/255 to nearest Q20.
    }
    // LCD shader sampling retains the original Q20 UV and source bytes, using
    // twelve fractional bits for each bilinear weight. Its normalized error is
    // at most 513/Q20 (under 0.00049), before the authored shader equation. The
    // complete weighted sum fits uint32: 255*4096*4096 < UINT32_MAX. This avoids
    // repeated 64-bit interpolation products on ARM926 without resizing a field.
    int sample_lcd(const Coordinates& c, unsigned channel) const {
        const int p00 = int(texel(c.at[0] + channel)), p10 = int(texel(c.at[1] + channel));
        const int p01 = int(texel(c.at[2] + channel)), p11 = int(texel(c.at[3] + channel));
        const unsigned fx = c.fx >> 8, fy = c.fy >> 8;
        const int a = p00 * 4096 + (p10 - p00) * int(fx), b = p01 * 4096 + (p11 - p01) * int(fx);
        const uint32_t weighted = (uint32_t(a) << 12) + uint32_t(b - a) * fy;
        return int((weighted + 2040u) / 4080u);
    }
    int sample_lcd(int u, int v, unsigned channel, bool repeat) const {
        return sample_lcd(coordinates(u, v, repeat), channel);
    }
    void sample_lcd_rg(int u, int v, bool repeat, int& r, int& g) const {
        const auto c = coordinates(u, v, repeat);
        r = sample_lcd(c, 0);
        g = sample_lcd(c, 1);
    }
    void sample_rg(int u, int v, bool repeat, int& r, int& g) const {
        const auto c = coordinates(u, v, repeat);
        r = sample(c, 0);
        g = sample(c, 1);
    }
};
struct Model {
    int q[32]{};
    void read(const uint8_t* p) {
        for (unsigned i = 0; i < 32; ++i)
            q[i] = integer(p + 4 * i);
    }
};
struct Pixel {
    unsigned inner = 0, alpha = 0;
};
inline unsigned smooth(int value, int lo, int hi, unsigned inverse) {
    if (value <= lo)
        return 0;
    if (value >= hi)
        return 1048576;
    const unsigned q = unsigned(uint64_t(unsigned(value - lo)) * inverse >> 24);
    const unsigned square = unsigned(uint64_t(q) * q >> 20);
    return unsigned(uint64_t(square) * (3145728 - 2 * q) >> 20);
}
struct Shader {
    const Field* field = nullptr;
    const Model* model = nullptr;
    int pan[4][2]{};
    unsigned inverse[2]{};
    void bind(const Field* f, const Model& m, uint32_t clock) {
        field = f;
        model = &m;
        const unsigned time = clock % 20000;
        for (unsigned i = 0; i < 4; ++i)
            for (unsigned j = 0; j < 2; ++j)
                pan[i][j] = int(int64_t(m.q[i * 5 + 2 + j]) * time / 16000);
        for (unsigned i = 0; i < 2; ++i)
            inverse[i] =
                unsigned((uint64_t(1048576) << 24) / unsigned(m.q[29 + 2 * i] - m.q[28 + 2 * i]));
    }
    Pixel sample(int u, int v) const {
        const auto& q = model->q;
        const int n1 = scaled(
            field[1].sample(scaled(u, q[0]) + pan[0][0], scaled(v, q[1]) + pan[0][1], 2, true),
            q[4]);
        const int n2 = scaled(
            field[0].sample(scaled(u, q[5]) + pan[1][0], scaled(v, q[6]) + pan[1][1], 0, true),
            q[9]);
        const int ax = scaled(u, q[10]) + pan[2][0], ay = scaled(v, q[11]) + pan[2][1];
        const int bx = scaled(u, q[15]) + pan[3][0], by = scaled(v, q[16]) + pan[3][1];
        const int zigzag = field[4].sample(bx, by, 0, true);
        int dx = scaled(field[1].sample(ax, ay, 0, true), q[14]) + scaled(zigzag, q[19]);
        int dy = scaled(field[1].sample(ax, ay, 1, true), q[14]) + scaled(zigzag, q[19]);
        const int mask =
            1048576 - field[3].sample(scaled(u, q[20]) + (q[22] >> 4),
                                      scaled(v - 41943, q[21]) + 41943 + (q[23] >> 4), 0, false);
        const int dist = field[3].sample(scaled(u, q[24]) + (q[26] >> 4),
                                         scaled(v, q[25]) + (q[27] >> 4), 0, false);
        dx = product(dx, dist);
        dy = product(dy, dist);
        const int base = field[2].sample(u + dx, v + dy, 0, false);
        const int strength = product(product(n1, n2) + mask, base) * 16;
        return {smooth(strength, q[28], q[29], inverse[0]),
                smooth(strength, q[30], q[31], inverse[1])};
    }
};
struct Table {
    const uint8_t* data = nullptr;
    template <class View> bool read(const View& v) {
        data = nullptr;
        if (!v.pixels || v.format || v.w != 320 || v.h != (Length + 639) / 640)
            return false;
        const auto* p = v.pixels;
        if (p[0] != 'A' || p[1] != 'F' || p[2] != 'R' || p[3] != '1' || word(p + 4) != 1 ||
            word(p + 6) != Nodes || word(p + 8) != Frames || word(p + 10) != Dynamic ||
            word(p + 12) != Models || word(p + 14) != Commands || dword(p + 16) != NodesAt ||
            dword(p + 20) != ModelsAt || dword(p + 24) != DeltasAt || dword(p + 28) != OrderAt ||
            dword(p + 32) != Length || word(p + 36) != FirstImage || word(p + 38) != PageCount ||
            dword(p + 40) != 20000 || dword(p + 44) != 16667)
            return false;
        if (dword(p + 48) > 1)
            return false;
        for (unsigned i = 52; i < 64; ++i)
            if (p[i])
                return false;
        unsigned dynamic = 0;
        for (unsigned i = 0; i < Nodes; ++i) {
            const auto* r = p + NodesAt + i * NodeBytes;
            if (r[36] >= Models || r[37] > 1 || (r[38] != 255 && r[38] >= Dynamic) || r[39])
                return false;
            for (unsigned a = 0; a < 6; ++a) {
                const int q = integer(r + a * 4);
                if (q <= -8388608 || q >= 8388608 || (a >= 2 && (q <= -32768 || q >= 32768)))
                    return false;
            }
            if (r[38] != 255 && r[38] != dynamic++)
                return false;
        }
        if (dynamic != Dynamic)
            return false;
        for (unsigned i = 0; i < Models; ++i) {
            const auto* r = p + ModelsAt + i * 128;
            for (unsigned k = 0; k < 32; ++k) {
                const int q = integer(r + 4 * k);
                if (q < -67108864 || q > 67108864)
                    return false;
            }
            if (integer(r + 112) < 0 || integer(r + 116) - integer(r + 112) < 65536 ||
                integer(r + 116) > 16777216 || integer(r + 120) < 0 ||
                integer(r + 124) - integer(r + 120) < 65536 || integer(r + 124) > 16777216)
                return false;
        }
        unsigned counts[3] = {};
        for (unsigned i = 0; i < Commands; ++i) {
            const unsigned command = word(p + OrderAt + i * 2), kind = command >> 14;
            if (kind >= 3 || (command & 16383) != counts[kind]++)
                return false;
        }
        if (counts[0] != 90 || counts[1] != 74 || counts[2] != 237)
            return false;
        for (unsigned i = Length; i < 320 * ((Length + 639) / 640) * 2; ++i)
            if (p[i])
                return false;
        data = p;
        return true;
    }
    unsigned frame(uint32_t clock) const {
        return (clock % 16667) / 33;
    }
    const uint8_t* node(unsigned i) const {
        return data + NodesAt + i * NodeBytes;
    }
    int coordinate(unsigned i, unsigned axis, unsigned frame) const {
        const auto* p = node(i);
        int q = integer(p + axis * 4);
        if (p[38] != 255)
            q += int16_t(word(data + DeltasAt + (p[38] * Frames + frame) * 12 + axis * 2));
        return q;
    }
    unsigned command(unsigned i) const {
        return word(data + OrderAt + i * 2);
    }
};
} // namespace spire::ancient_fire
