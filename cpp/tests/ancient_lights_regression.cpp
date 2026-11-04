#include <array>
#include <cstdio>
#include <cstdlib>
#include "ancient_lights.hpp"
using namespace spire::ancient_lights;
static unsigned checks;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::fprintf(stderr, "Ancient lights:%d %s\n", __LINE__, #x);                          \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)
struct Picture {
    const uint8_t* pixels;
    unsigned w, h, format;
    const void *alpha = nullptr, *palette = nullptr;
};
static void word(uint8_t* p, unsigned n) {
    p[0] = uint8_t(n);
    p[1] = uint8_t(n >> 8);
}
static void integer(uint8_t* p, int n) {
    const uint32_t value = uint32_t(n);
    word(p, value);
    word(p + 2, value >> 16);
}
int main() {
    std::array<uint8_t, 106496> raw{};
    for (unsigned scene = 0; scene < 2; ++scene) {
        raw.fill(0);
        const unsigned nodes = scene ? 90 : 10, dynamic = scene ? 17 : 3,
                       frames = scene ? 506 : 546, duration = scene ? 16667 : 18000;
        const unsigned length = 32 + nodes * 32 + dynamic * frames * 12;
        const unsigned header[] = {0x4c41,
                                   1,
                                   scene,
                                   nodes,
                                   dynamic,
                                   frames,
                                   33,
                                   duration,
                                   0,
                                   32,
                                   32,
                                   32 + nodes * 32,
                                   length & 65535,
                                   length >> 16,
                                   scene ? 65535 : FirstImage,
                                   0};
        for (unsigned i = 0; i < 16; ++i)
            word(raw.data() + i * 2, header[i]);
        for (unsigned i = 0; i < nodes; ++i) {
            auto* p = raw.data() + 32 + i * 32;
            integer(p, -1024);
            integer(p + 4, 512);
            integer(p + 8, 10240);
            integer(p + 12, 8192);
            integer(p + 16, -200);
            integer(p + 20, 30);
            p[28] = uint8_t(scene && i == nodes - 1 ? 1 : 2);
            p[29] = uint8_t(i < dynamic ? i : 255);
            p[30] = uint8_t(scene || i >= 7);
            p[31] = uint8_t(scene && i == nodes - 1);
        }
        for (unsigned node = 0; node < dynamic; ++node)
            for (unsigned frame = 0; frame < frames; ++frame)
                word(raw.data() + 32 + nodes * 32 + (node * frames + frame) * 12,
                     unsigned(-int(frame)));
        Picture picture{raw.data(), 256, (length + 511) / 512, 0};
        Table table;
        CHECK(table.read(picture, scene));
        for (unsigned frame = 0; frame < frames; ++frame)
            for (unsigned node = 0; node < nodes; ++node) {
                CHECK(table.coordinate(table.node(node), 0, frame) ==
                      -1024 - (node < dynamic ? int(frame) : 0));
                CHECK(table.coordinate(table.node(node), 4, frame) == -200);
            }
        for (uint32_t clock : {0u, 33u, 17985u, 18000u, 16667u, 0xffffffffu}) {
            CHECK(table.frame(clock, false) == (clock % duration) / 33);
            CHECK(table.frame(clock, true) == 0);
        }
        for (unsigned at : {0u, 2u, 4u, 10u, 18u, 20u, 22u, 28u, 30u, 61u, 62u}) {
            raw[at] ^= 1;
            CHECK(!table.read(picture, scene));
            raw[at] ^= 1;
        }
        integer(raw.data() + 32 + 8, 0x7fffffff);
        CHECK(!table.read(picture, scene));
    }
    const uint8_t low[] = {0, 0, 0, 128}, high[] = {255, 255, 255, 127},
                  minus[] = {255, 255, 255, 255};
    CHECK(integer(low) == (-2147483647 - 1));
    CHECK(integer(high) == 2147483647);
    CHECK(integer(minus) == -1);
    std::printf("Ancient light typed bounds/clocks: %u checks passed\n", checks);
}
