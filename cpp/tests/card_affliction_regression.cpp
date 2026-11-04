#include "card_affliction_shader.hpp"
#include <array>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire::card_affliction;
static unsigned checks;
#define CHECK(q)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(q)) {                                                                                \
            std::fprintf(stderr, "Card material:%d %s\n", __LINE__, #q);                           \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)
static void put(uint8_t* p, unsigned v, unsigned bytes = 4) {
    for (unsigned i = 0; i < bytes; ++i)
        p[i] = uint8_t(v >> (i * 8));
}
static std::vector<uint8_t> field(unsigned w, unsigned h, unsigned c) {
    const unsigned cols = (w + 31) / 32, rows = (h + 31) / 32, count = cols * rows,
                   data = (32 + count * 2 + 3) & ~3u;
    std::vector<uint8_t> raw(data + count * 1024 * c);
    memcpy(raw.data(), "ACF1", 4);
    const unsigned header[] = {2, w, h, c, cols, rows};
    for (unsigned i = 0; i < 6; ++i)
        put(raw.data() + 4 + i * 2, header[i], 2);
    put(raw.data() + 16, 32);
    put(raw.data() + 20, data);
    put(raw.data() + 24, count);
    put(raw.data() + 28, unsigned(raw.size()));
    for (unsigned tile = 0; tile < count; ++tile) {
        put(raw.data() + 32 + tile * 2, tile + 1, 2);
        for (unsigned channel = 0; channel < c; ++channel)
            for (unsigned y = 0; y < 32; ++y)
                for (unsigned x = 0; x < 32; ++x) {
                    const unsigned xx = tile % cols * 32 + x, yy = tile / cols * 32 + y;
                    if (xx < w && yy < h)
                        raw[data + (channel * count + tile) * 1024 + y * 32 + x] =
                            uint8_t(xx * 17 + yy * 31 + channel * 67);
                }
    }
    return raw;
}
static double original(const Field& f, int u, int v, unsigned ch, bool repeat) {
    auto wrap = [](int x, int n) {
        x %= n;
        return x < 0 ? x + n : x;
    };
    double x =
        (repeat ? double(unsigned(u) & (Q - 1)) / Q : double(clamp(u, 0, Q)) / Q) * f.width - .5;
    double y =
        (repeat ? double(unsigned(v) & (Q - 1)) / Q : double(clamp(v, 0, Q)) / Q) * f.height - .5;
    const int ix = int(std::floor(x)), iy = int(std::floor(y));
    const double fx = x - ix, fy = y - iy;
    auto pixel = [&](int xx, int yy) {
        return double(f.texel(
                   unsigned(repeat ? wrap(xx, int(f.width)) : clamp(xx, 0, int(f.width) - 1)),
                   unsigned(repeat ? wrap(yy, int(f.height)) : clamp(yy, 0, int(f.height) - 1)),
                   ch)) /
               255.;
    };
    return ((pixel(ix, iy) * (1 - fx) + pixel(ix + 1, iy) * fx) * (1 - fy) +
            (pixel(ix, iy + 1) * (1 - fx) + pixel(ix + 1, iy + 1) * fx) * fy) *
           Q;
}
int main() {
    auto raw = field(65, 47, 4);
    const uint8_t* pages[] = {raw.data()};
    Field f;
    CHECK(f.bind({pages, unsigned(raw.size())}, 0));
    for (unsigned y = 0; y < 47; ++y)
        for (unsigned x = 0; x < 65; ++x)
            for (unsigned ch = 0; ch < 4; ++ch)
                CHECK(f.texel(x, y, ch) == uint8_t(x * 17 + y * 31 + ch * 67));
    uint32_t rng = 47;
    for (unsigned i = 0; i < 30000; ++i) {
        rng = rng * 1664525u + 1013904223u;
        int u = int(rng & 4194303) - 1048576;
        rng = rng * 1664525u + 1013904223u;
        int v = int(rng & 4194303) - 1048576;
        for (bool repeat : {false, true})
            CHECK(std::abs(f.sample(u, v, i % 4, repeat) - original(f, u, v, i % 4, repeat)) < 1.1);
    }
    // Independent four-corner reference across sparse tile and physical page
    // boundaries. Guard bytes separate pages so a cross-page pointer shortcut
    // cannot accidentally pass merely because the host allocation is contiguous.
    const unsigned shapes[][3] = {{1, 1, 1}, {31, 33, 2}, {65, 47, 4}, {1024, 33, 4}};
    for (const auto& shape : shapes)
        for (unsigned offset : {0u, 131039u, 131071u}) {
            auto pixels = field(shape[0], shape[1], shape[2]);
            const unsigned count = word(pixels.data() + 12) * word(pixels.data() + 14);
            for (unsigned tile = 0; tile < count; ++tile)
                if (tile % 3 == 1)
                    put(pixels.data() + 32 + tile * 2, 0, 2);
            if (count > 1)
                put(pixels.data() + 32 + (count - 1) * 2, count + 1, 2);
            std::vector<uint8_t> bank(offset + pixels.size());
            std::memcpy(bank.data() + offset, pixels.data(), pixels.size());
            std::vector<std::array<uint8_t, PageBytes + 64>> storage((bank.size() + PageBytes - 1) /
                                                                     PageBytes);
            std::vector<const uint8_t*> pointers;
            for (unsigned i = 0; i < storage.size(); ++i) {
                storage[i].fill(205);
                const unsigned n =
                    unsigned(std::min(size_t(PageBytes), bank.size() - size_t(i) * PageBytes));
                std::memcpy(storage[i].data() + 32, bank.data() + size_t(i) * PageBytes, n);
                pointers.push_back(storage[i].data() + 32);
            }
            Field paged;
            CHECK(paged.bind({pointers.data(), unsigned(bank.size())}, offset));
            for (unsigned i = 0; i < 10000; ++i) {
                rng = rng * 1664525u + 1013904223u;
                int u = int(rng & 4194303) - 1048576;
                rng = rng * 1664525u + 1013904223u;
                int v = int(rng & 4194303) - 1048576;
                if (i < 5) {
                    u = int(i) * int(Q) / 2 - int(Q / 2);
                    v = int(Q) - u;
                }
                for (bool repeat : {false, true})
                    CHECK(std::abs(paged.sample(u, v, i % (shape[2] + 1), repeat) -
                                   original(paged, u, v, i % (shape[2] + 1), repeat)) < 1.1);
            }
        }
    const auto saved = raw;
    for (unsigned at : {0u, 4u, 6u, 8u, 10u, 12u, 14u, 16u, 20u, 24u, 28u}) {
        raw = saved;
        raw[at] ^= 128;
        pages[0] = raw.data();
        CHECK(!f.bind({pages, unsigned(raw.size())}, 0));
    }
    raw = saved;
    pages[0] = raw.data();
    CHECK(f.bind({pages, unsigned(raw.size())}, 0));
    // Prepared dynamic widths and constant-width shader calls must retain
    // the exact Q20 quotient, including both ends and the conservative fallback.
    for (int width :
         {256, 257, 511, 65536, 104858, 209715, 314573, 629146, 1048575, 1048576, 1048577}) {
        const auto reciprocal = smooth_reciprocal(width);
        for (int n = -1; n <= width + 1; ++n) {
            const int q = n <= 0 ? 0 : n >= width ? int(Q) : int(int64_t(n) * Q / width);
            const int expected = product(product(q, q), 3 * int(Q) - 2 * q);
            CHECK(smooth(n, 0, width, reciprocal) == expected);
            CHECK(smooth(n, 0, width) == expected);
        }
    }
    double angle_error = 0, radius_error = 0;
    for (int y = 0; y <= 128; ++y)
        for (int x = 0; x <= 128; ++x) {
            int r, t;
            polar(x * 8192, y * 8192, r, t);
            const double dx = x / 128. - .5, dy = y / 128. - .5;
            if (dx || dy) {
                double want = std::atan2(dy, dx) / (2 * 3.1416);
                want -= std::floor(want);
                double d = std::abs(t / double(Q) - want);
                d = std::min(d, 1 - d);
                angle_error = std::max(angle_error, d);
                CHECK(d < .00004);
            }
            double want = std::hypot(dx, dy) * 2;
            want -= std::floor(want);
            double d = std::abs(r / double(Q) - want);
            d = std::min(d, 1 - d);
            radius_error = std::max(radius_error, d);
            CHECK(d < .00004);
        }
    std::array<uint8_t, 256 * 4 * 4> curve{};
    for (unsigned n = 0; n < 256; ++n)
        for (unsigned ch = 0; ch < 4; ++ch)
            put(curve.data() + (n * 4 + ch) * 4,
                unsigned((int(n) * 4096 + int(ch) * 773) & 1048575));
    int32_t parameters[16] = {524288,  786432, 367002, 471859,  786432, 524288, 524288,
                              -104858, 314573, 0,      1048576, 35,     50};
    unsigned moving = 0;
    for (unsigned kind = 1; kind <= 7; ++kind) {
        Shader s;
        s.kind = kind;
        s.repeat = kind != Ringing;
        s.parameters = parameters;
        for (auto& ff : s.field)
            ff = &f;
        s.curve[0] = s.curve[1] = {curve.data(), 256, 4};
        for (uint32_t clock :
             {0u, 1u, 33u, 2499u, 5714u, 9999u, 19999u, 99999u, 199999u, 0xffffffffu}) {
            s.prepare(clock);
            const auto a = s.sample(345678, 789123);
            CHECK(a.a >= 0 && a.a <= 2 * int(Q));
            if (clock < 1000000) {
                s.prepare(clock + 200000);
                const auto b = s.sample(345678, 789123);
                CHECK(a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a);
            }
        }
        s.prepare(0);
        auto a = s.sample(345678, 789123);
        s.prepare(73);
        auto b = s.sample(345678, 789123);
        moving += memcmp(&a, &b, sizeof a) != 0;
    }
    CHECK(moving >= 5);
    std::printf("Card continuous materials: %u checks, polar angle bound %.9g turns, radius %.9g, "
                "Field=%zu Shader=%zu\n",
                checks, angle_error, radius_error, sizeof(Field), sizeof(Shader));
}
