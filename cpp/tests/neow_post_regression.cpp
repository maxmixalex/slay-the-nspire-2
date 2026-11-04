#include "neow_post.hpp"
#include "presentation_blend.hpp"
#include <array>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <chrono>
#include <string>
namespace spire {
ForegroundWorkspace foreground_workspace;
uint8_t resource_transfer_uses = 0;
unsigned foreground_scratch_top = 0;
} // namespace spire
using namespace spire;
using namespace spire::neow_post;
static unsigned checks;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::fprintf(stderr, "Neow post:%d %s\n", __LINE__, #x);                               \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)
struct Picture {
    const uint8_t* pixels;
    const uint8_t* alpha;
    const uint8_t* palette;
    unsigned w, h, format;
};
static std::vector<uint8_t> words(bool still) {
    const unsigned h = still ? StaticHeight : FullHeight, mask = still ? StaticMaskAt : MaskAt,
                   lod = still ? mask + 10000 : LodAt;
    std::vector<uint8_t> raw(Width * h * 2);
    auto put = [&](unsigned at, unsigned n, unsigned count = 2) {
        for (unsigned b = 0; b < count; ++b)
            raw[at + b] = uint8_t(n >> (b * 8));
    };
    const unsigned header[] = {
        0x4e57, 1,    FirstImage, AuxiliaryFirst,  Width,   h,          128, 100, NoiseAt, mask,
        lod,    1000, 15,         unsigned(still), NoiseAt, StaticCount};
    for (unsigned i = 0; i < 16; ++i)
        put(i * 2, header[i]);
    const int geometry[] = {-6069744, -21062688, 83839, 145269};
    for (unsigned i = 0; i < 4; ++i)
        put(32 + i * 4, unsigned(geometry[i]), 4);
    const unsigned bounds[] = {Left, Top, Right, Bottom};
    for (unsigned i = 0; i < 4; ++i)
        put(48 + i * 2, bounds[i]);
    for (unsigned i = 0; i < 1025; ++i)
        put(lod + i * 2,
            i <= 256 ? 0 : unsigned(std::lround(std::log2(std::sqrt(i / 256.)) * 256)));
    for (unsigned i = 0; i < 10000; ++i)
        raw[mask + i] = uint8_t(i % 256);
    if (still)
        for (unsigned i = 0; i < StaticCount; ++i)
            put(NoiseAt + i * 2, (i * 29) % (255 * 256 + 1));
    else
        for (unsigned y = 0; y < 128; ++y)
            for (unsigned x = 0; x < 128; ++x)
                raw[NoiseAt + y * 128 + x] =
                    uint8_t(127 + int(55 * std::sin(x * .0490873852) * std::cos(y * .0490873852)));
    return raw;
}
static std::vector<uint8_t> read(const char* path) {
    FILE* f = std::fopen(path, "rb");
    CHECK(f);
    CHECK(!std::fseek(f, 0, SEEK_END));
    const auto n = std::ftell(f);
    CHECK(n > 0);
    std::rewind(f);
    std::vector<uint8_t> d(size_t(n), 0);
    CHECK(std::fread(d.data(), 1, d.size(), f) == d.size());
    std::fclose(f);
    return d;
}
static void original_render(uint16_t* pixels, int pitch, const Snapshot& source, const Table& table,
                            uint32_t clock) {
    // speed1.0 and repeat_enable give an exact one-second noise period. Screen
    // capture is independent of that phase and happens once after every layer.
    const unsigned phase = unsigned(uint64_t(clock % 1000u) * 16777216u / 1000u);
    for (unsigned y = Top; y < Bottom; y += 2)
        for (unsigned x = Left; x < Right; x += 2) {
            int sx[4], sy[4];
            for (unsigned i = 0; i < 4; ++i) {
                const unsigned xx = x + (i & 1), yy = y + (i >> 1), n = table.noise(xx, yy, phase);
                // RoundedQ16 factors encode source strength.015 against320x240 pixels;
                // the maximum displacement quantization is below0.00004LCDpixels.
                sx[i] = int(xx * 65536) + int((int64_t(n) * 315806 + 32768) >> 16) - 157286;
                sy[i] = int(yy * 65536) + int((int64_t(n) * 236855 + 32768) >> 16) - 117965;
            }
            for (unsigned i = 0; i < 4; ++i) {
                const unsigned xx = x + (i & 1), yy = y + (i >> 1), alpha = table.alpha(xx, yy);
                if (!alpha)
                    continue;
                const unsigned left = i & 2, top = i & 1;
                const unsigned lod = table.lod(sx[left + 1] - sx[left], sy[left + 1] - sy[left],
                                               sx[top + 2] - sx[top], sy[top + 2] - sy[top]);
                uint16_t color = sample(source.full, 320, 240, sx[i], sy[i]);
                if (lod)
                    color = mix(
                        color,
                        sample(source.mip1, 160, 120, (sx[i] >> 1) - 16384, (sy[i] >> 1) - 16384),
                        lod);
                pixels[yy * pitch + xx] =
                    alpha == 255 ? color : blend(source.full[yy * 320 + xx], color, alpha);
            }
        }
}
int main(int argc, char** argv) {
    // Exhaust each channel pair and all Q8 fractions. Complementary other
    // channels exercise packed-lane borrow/carry without a shared arithmetic
    // implementation in the oracle, including f=256 endpoints.
    for (unsigned a = 0; a < 64; ++a)
        for (unsigned b = 0; b < 64; ++b)
            for (unsigned f = 0; f <= 256; ++f) {
                const uint16_t x = uint16_t(((a & 31) << 11) | (a << 5) | (31 - (a & 31)));
                const uint16_t y = uint16_t(((b & 31) << 11) | (b << 5) | (31 - (b & 31)));
                const auto channel = [&](unsigned u, unsigned v) {
                    return (u * (256 - f) + v * f + 128) >> 8;
                };
                const auto expected = uint16_t((channel(x >> 11, y >> 11) << 11) |
                                               (channel((x >> 5) & 63, (y >> 5) & 63) << 5) |
                                               channel(x & 31, y & 31));
                CHECK(mix(x, y, f) == expected);
            }

    for (unsigned a = 0; a <= 255; ++a)
        for (unsigned c = 0; c < 64; ++c) {
            const uint16_t x = uint16_t(((c & 31) << 11) | (c << 5) | (31 - (c & 31))),
                           y = uint16_t(~x);
            CHECK(blend(x, y, a) == presentation::source_over_rgb565(x, y, a));
        }
    std::vector<uint16_t> original(320 * 240), actual, expected;
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x)
            original[y * 320 + x] = uint16_t((((x * 7 + y) & 31) << 11) |
                                             (((x + y * 3) & 63) << 5) | ((x * 3 + y * 5) & 31));
    if (argc > 3) {
        const auto b = read(argv[3]);
        CHECK(b.size() == original.size() * 2);
        std::memcpy(original.data(), b.data(), b.size());
    }
    for (bool still : {false, true}) {
        auto raw = argc > 2 ? read(argv[still ? 2 : 1]) : words(still);
        Table table;
        Picture p{raw.data(), nullptr, nullptr, Width, still ? StaticHeight : FullHeight, 0};
        CHECK(table.read(p, still));
        CHECK(!Table{}.read(p, !still));
        auto changed = raw;
        changed[26] ^= 1;
        CHECK(!Table{}.read(Picture{changed.data(), nullptr, nullptr, p.w, p.h, 0}, still));
        CoverageCache coverage;
        CHECK(coverage.prepare(&table, 1, table));
        CHECK(!coverage.prepare(&table, 1, table));
        for (unsigned y = Top; y < Bottom; ++y)
            for (unsigned x = Left; x < Right; ++x)
                CHECK(coverage.pixels[(y - Top) * (Right - Left) + x - Left] == table.alpha(x, y));
        auto empty = raw;
        std::memset(empty.data() + (still ? StaticMaskAt : MaskAt), 0, 10000);
        Table empty_table;
        CHECK(empty_table.read(Picture{empty.data(), nullptr, nullptr, p.w, p.h, 0}, still));
        CHECK(coverage.prepare(&table, 2, empty_table));
        for (unsigned value : coverage.pixels)
            CHECK(value == 0);
        CHECK(coverage.prepare(&empty_table, 2, table));
        CHECK(!coverage.prepare(&empty_table, 2, table));
        const auto alternate = words(!still);
        Table alternate_table;
        CHECK(alternate_table.read(Picture{alternate.data(), nullptr, nullptr, Width,
                                           still ? FullHeight : StaticHeight, 0},
                                   !still));
        CHECK(coverage.prepare(&empty_table, 2, alternate_table));
        CHECK(coverage.prepare(&empty_table, 2, table));
        Snapshot independent;
        snapshot(independent, original.data(), 320);
        const auto before = independent;
        for (unsigned clock : {0u, 1u, 17u, 499u, 999u, 1000u, 3600000u, 0xffffffffu}) {
            actual = original;
            expected = original;
            original_render(expected.data(), 320, independent, table, still ? 0 : clock);
            std::memset(foreground_workspace.transient.transfer.encoded.data, 0xcd,
                        ResourceTransferCapacity);
            CHECK(apply(actual.data(), 320, table, clock));
            auto cached = original;
            CHECK(apply<true>(cached.data(), 320, table, clock, coverage.pixels));
            CHECK(cached == actual);
            for (unsigned pixel = 0; pixel < actual.size(); ++pixel) {
                CHECK(std::abs(int(actual[pixel] >> 11) - int(expected[pixel] >> 11)) <= 1);
                CHECK(std::abs(int((actual[pixel] >> 5) & 63) - int((expected[pixel] >> 5) & 63)) <=
                      2);
                CHECK(std::abs(int(actual[pixel] & 31) - int(expected[pixel] & 31)) <= 1);
            }
            CHECK(!resource_transfer_uses);
            CHECK(!std::memcmp(&independent, &before, sizeof before));
            for (unsigned y = 0; y < 240; ++y)
                for (unsigned x = 0; x < 320; ++x)
                    if (x < Left || x >= Right || y < Top)
                        CHECK(actual[y * 320 + x] == original[y * 320 + x]);
        }
        // Extreme displacement and forced mip1 exercise every side of the
        // bounded snapshot, including a padded destination stride. Poison unused
        // arena bytes so a missing source texel cannot pass by retaining old data.
        for (unsigned extreme : {0u, 65280u}) {
            auto edge = raw;
            const unsigned mask = still ? StaticMaskAt : MaskAt,
                           lod = still ? StaticMaskAt + 10000 : LodAt;
            std::memset(edge.data() + mask, 255, 10000);
            if (still)
                for (unsigned i = 0; i < StaticCount; ++i) {
                    edge[NoiseAt + i * 2] = uint8_t(extreme);
                    edge[NoiseAt + i * 2 + 1] = uint8_t(extreme >> 8);
                }
            else
                std::memset(edge.data() + NoiseAt, extreme ? 255 : 0, 128 * 128);
            for (unsigned i = 0; i < 1025; ++i) {
                edge[lod + 2 * i] = 0;
                edge[lod + 2 * i + 1] = 1;
            }
            Table edge_table;
            CHECK(edge_table.read(Picture{edge.data(), nullptr, nullptr, p.w, p.h, 0}, still));
            std::vector<uint16_t> padded(327 * 240, 0x6b4d);
            for (unsigned y = 0; y < 240; ++y)
                for (unsigned x = 0; x < 320; ++x)
                    padded[y * 327 + x] = original[y * 320 + x];
            auto edge_expected = padded;
            Snapshot edge_source;
            snapshot(edge_source, padded.data(), 327);
            original_render(edge_expected.data(), 327, edge_source, edge_table, 0);
            std::memset(foreground_workspace.transient.transfer.encoded.data, 0xa7,
                        ResourceTransferCapacity);
            CHECK(apply(padded.data(), 327, edge_table, 0));
            CHECK(padded == edge_expected);
        }
        for (auto use : {ResourceTransferUse::Bytes, ResourceTransferUse::Save,
                         ResourceTransferUse::Codec, ResourceTransferUse::Map}) {
            ResourceTransferLease held(use);
            CHECK(held);
            actual = original;
            CHECK(!apply(actual.data(), 320, table, 0));
            CHECK(actual == original);
            CHECK(resource_transfer_uses == unsigned(use));
        }
        CHECK(!resource_transfer_uses);
        Table invalid;
        CHECK(!apply(actual.data(), 320, invalid, 0));
        CHECK(!resource_transfer_uses);
        CHECK(!apply(nullptr, 320, table, 0));
        CHECK(!apply(actual.data(), 319, table, 0));
        CHECK(!apply<true>(actual.data(), 320, table, 0));
        CHECK(!resource_transfer_uses);
        const auto start = std::chrono::steady_clock::now();
        for (unsigned n = 0; n < 120; ++n) {
            actual = original;
            CHECK(apply(actual.data(), 320, table, n * 33));
        }
        const auto ms =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
                .count() /
            120;
        std::printf(
            "Neow post %s: %.3f ms host kernel including snapshot+mip; %zu snapshot bytes\n",
            still ? "static" : "animated", ms, sizeof(Snapshot));
        if (argc > 4) {
            actual = original;
            CHECK(apply(actual.data(), 320, table,
                        argc > 5 ? unsigned(std::strtoul(argv[5], nullptr, 10)) : 0));
            const auto name = std::string(argv[4]) + (still ? "-static.rgb565" : "-full.rgb565");
            FILE* f = std::fopen(name.c_str(), "wb");
            CHECK(f);
            CHECK(std::fwrite(actual.data(), 2, actual.size(), f) == actual.size());
            std::fclose(f);
        }
    }
    std::printf("Neow post: %u checks passed; no persistent allocation or content callback\n",
                checks);
}
