#ifdef NDEBUG
#undef NDEBUG
#endif
#include "neow_particles.hpp"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <vector>
using namespace spire::neow_particles;
struct Picture {
    const uint8_t* pixels;
    int w, h, format;
    const uint8_t *alpha, *palette;
};
static void put(std::vector<uint8_t>& v, unsigned offset, uint32_t value, unsigned bytes = 2) {
    for (unsigned i = 0; i < bytes; ++i)
        v[offset + i] = uint8_t(value >> (i * 8));
}
int main() {
    std::vector<uint8_t> bytes(Bytes, 0);
    bytes[0] = 'N';
    bytes[1] = 'N';
    bytes[2] = 'P';
    bytes[3] = '1';
    const unsigned short fields[] = {1, AtlasFirst, 4, 32, 32, 3};
    for (unsigned i = 0; i < 6; ++i)
        put(bytes, 4 + i * 2, fields[i]);
    const unsigned offsets[] = {64, 1088, 2112, 2624, 2880, 2910, 3422, 4446};
    for (unsigned i = 0; i < 8; ++i)
        put(bytes, 16 + i * 4, offsets[i], 4);
    for (unsigned i = 0; i < 256; ++i) {
        put(bytes, 64 + i * 4, uint32_t(int(i) * -10000), 4);
        put(bytes, 1088 + i * 4, i * 200, 4);
        put(bytes, 2112 + i * 2, 4096);
        bytes[2624 + i] = uint8_t(i);
    }
    put(bytes, 2880, 0);
    put(bytes, 2890, 16963);
    put(bytes, 2900, 65535);
    for (unsigned i = 0; i < 3; ++i)
        for (unsigned channel = 0; channel < 4; ++channel)
            put(bytes, 2882 + i * 10 + channel * 2, i == 1 ? 65535 : channel == 3 ? 0 : 32000);
    Parameters p;
    assert(p.read(Picture{bytes.data(), 256, 9, 0, nullptr, nullptr}));
    unsigned checks = 1;
    for (unsigned offset :
         {0u, 4u, 6u, 8u, 10u, 12u, 14u, 16u, 20u, 24u, 28u, 32u, 36u, 40u, 44u, 56u, 2880u}) {
        auto bad = bytes;
        bad[offset] ^= 1;
        assert(!Parameters{}.read(Picture{bad.data(), 256, 9, 0, nullptr, nullptr}));
        ++checks;
    }
    // Compare closed-form gravity against an independent step-by-step source
    // force/velocity integrator, plus interpolation of its adjacent positions.
    for (unsigned age = 0; age < 10000; ++age) {
        const unsigned ticks = age * 30 / 1000;
        const double fraction = double(age * 30 % 1000) / 1000;
        double x = 0, velocity = 0;
        for (unsigned step = 0; step < ticks; ++step) {
            velocity += 50. / 30;
            x += velocity / 30;
        }
        const double expected = x + (velocity + 50. / 30) / 30 * fraction;
        const auto a = fog(p, age, 0x12345678u, 0, 0), b = fog(p, age, 0x12345678u, 2, 0);
        assert(fabs(a.x / 256. - expected) < 1. / 256. + 1e-7 && a.x == -b.x);
        ++checks;
        assert(a.scale >= 4 * 4096 && a.scale <= 6 * 4096 && a.atlas < 4 && a.gray <= 255);
        ++checks;
    }
    for (unsigned phase = 0; phase <= 65535; ++phase)
        for (unsigned channel = 0; channel < 4; ++channel) {
            const double center = 16963. / 65535, where = phase / 65535.;
            const double first = channel == 3 ? 0 : 32000;
            const double expected = where <= center
                                        ? first + (65535 - first) * where / center
                                        : 65535 + (first - 65535) * (where - center) / (1 - center);
            assert(fabs(double(p.star_color(channel, phase)) - expected) < 1.00001);
            ++checks;
        }
    for (uint32_t clock :
         {0u, 1u, 33u, 4999u, 5000u, 9999u, 10000u, 99999u, 0xfffffff0u, 0xffffffffu})
        for (unsigned i = 0; i < 10; ++i) {
            const auto a = star(clock, 0x87654321u, i), b = star(clock, 0x87654321u, i);
            assert(a.x == b.x && a.y == b.y && a.scale == b.scale && a.phase == b.phase);
            assert(a.phase < 65536 && a.scale >= 4096 && a.scale <= 8192);
            assert(a.x > -800 * 256 && a.x < 2800 * 256 && a.y > -500 * 256 && a.y < 1700 * 256);
            checks += 3;
        }
    printf("Neow source cloud/CPU-star numeric checks: %u\n", checks);
}
