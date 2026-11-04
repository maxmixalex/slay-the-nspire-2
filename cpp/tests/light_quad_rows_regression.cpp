#include "light_quad_rows.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
static unsigned checks;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::fprintf(stderr, "light row:%d %s\n", __LINE__, #x);                               \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)
int main() {
    uint32_t random = 17;
    auto next = [&]() {
        random = random * 1664525u + 1013904223u;
        return random;
    };
    for (unsigned trial = 0; trial < 10000; ++trial) {
        int dx = int(next() % 1000001) - 500000, dy = int(next() % 1000001) - 500000;
        int sine = int(next() % 2049) - 1024, cosine = int(next() % 2049) - 1024;
        int width = int(next() % 150000) + 1, height = int(next() % 150000) + 1;
        int shear = int(next() % 200001) - 100000;
        spire::LightQuadRows rows(dx, dy, sine, cosine, width, height, shear);
        for (unsigned y = 0; y < 240; ++y, dy += 256, rows.next()) {
            const int64_t vy = -int64_t(dx) * sine + int64_t(dy) * cosine;
            const int u =
                32768 +
                int(((int64_t(dx) * cosine + int64_t(dy) * sine) * 64 - vy * shear / 1024) / width);
            const int v = 32768 + int(vy * 64 / height);
            CHECK(rows.x() == u);
            CHECK(rows.y() == v);
        }
    }
    for (unsigned trial = 0; trial < 30000; ++trial) {
        const int u = trial & 1 ? int(next()) : int(next() % 200001) - 100000,
                  v = trial & 1 ? int(next()) : int(next() % 200001) - 100000;
        const int du = trial < 10  ? int(trial) - 5
                       : trial & 1 ? int(next())
                                   : int(next() % 4001) - 2000;
        const int dv = trial < 10  ? 5 - int(trial)
                       : trial & 1 ? int(next())
                                   : int(next() % 4001) - 2000;
        const unsigned low_u = next() % 65536, low_v = next() % 65536;
        const unsigned extent_u = 1 + next() % (65536 - low_u),
                       extent_v = 1 + next() % (65536 - low_v);
        const unsigned count = 1 + next() % 320;
        spire::LightQuadSpan su(du), sv(dv);
        unsigned first = 0, last = count;
        su.clip(u, low_u, extent_u, count, first, last);
        sv.clip(v, low_v, extent_v, count, first, last);
        for (unsigned x = 0; x < count; ++x) {
            const int64_t a = int64_t(u) + int64_t(du) * x, b = int64_t(v) + int64_t(dv) * x;
            CHECK((first < last && x >= first && x < last) ==
                  (a >= low_u && a < int64_t(low_u) + extent_u && b >= low_v &&
                   b < int64_t(low_v) + extent_v));
        }
        if (su.step) {
            const unsigned n = next();
            CHECK(su.divide(n) == n / su.step);
        }
    }
    std::printf("Light UV rows: %u exact checks including both signs and zero crossings\n", checks);
}
