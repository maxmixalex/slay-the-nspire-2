#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "tanx_scene.hpp"
#include "tanx_particles.hpp"
using namespace spire;
static unsigned checks;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::fprintf(stderr, "Tanx:%d %s\n", __LINE__, #x);                                    \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)
static unsigned smooth(double a, double b) {
    const double x = a * b / (255.0 * 255.0);
    return unsigned(std::floor(255 * x * x * (3 - 2 * x) + .5));
}
int main() {
    std::array<uint8_t, 65536> lookup{};
    std::array<uint8_t, 200> gradient{};
    std::array<uint8_t, 160000> pixels{};
    for (unsigned a = 0; a < 256; ++a)
        for (unsigned b = 0; b < 256; ++b)
            lookup[a * 256 + b] = uint8_t(smooth(a, b));
    for (unsigned y = 0; y < 800; ++y)
        for (unsigned x = 0; x < 200; ++x)
            pixels[y * 200 + x] = uint8_t((x * 37 + y * 53) ^ (y >> 3));
    for (unsigned y = 0; y < 200; ++y)
        gradient[y] = uint8_t(y * 19);
    tanx_scene::Smoke smoke{
        {pixels.data(), pixels.data() + 40000, pixels.data() + 80000, pixels.data() + 120000},
        gradient.data(),
        lookup.data()};
    for (unsigned a = 0; a < 256; ++a)
        for (unsigned b = 0; b < 256; ++b)
            CHECK(smoke.smooth(a * 256, b * 256) == lookup[a * 256 + b]);
    for (int y : {-801, -800, -1, 0, 198, 199, 200, 201, 399, 400, 599, 600, 798, 799, 800, 801})
        for (int x : {-201, -200, -1, 0, 1, 198, 199, 200, 201}) {
            int xx = x % 200, yy = y % 800;
            if (xx < 0)
                xx += 200;
            if (yy < 0)
                yy += 800;
            CHECK(smoke.at(x, y) == pixels[yy * 200 + xx]);
        }
    unsigned max_alpha_error = 0;
    for (unsigned a = 0; a < 65281; a += 113)
        for (unsigned b = 0; b < 65281; b += 797) {
            const unsigned actual = smoke.smooth(a, b), expected = smooth(a / 256.0, b / 256.0);
            const unsigned error = actual > expected ? actual - expected : expected - actual;
            if (error > max_alpha_error)
                max_alpha_error = error;
            CHECK(error <= 1);
        }
    const auto reference = [&](int u, int v, unsigned scroll) {
        const double x = double(u & 65535) * 200 / 65536 - .5,
                     y = double((v + int(scroll)) & 65535) * 800 / 65536 - .5;
        const int ix = int(std::floor(x)), iy = int(std::floor(y));
        const double fx = x - ix, fy = y - iy;
        const double a = (smoke.at(ix, iy) * (1 - fx) + smoke.at(ix + 1, iy) * fx) * (1 - fy) +
                         (smoke.at(ix, iy + 1) * (1 - fx) + smoke.at(ix + 1, iy + 1) * fx) * fy;
        const double gy = double(v & 65535) * 200 / 65536 - .5;
        int gi = int(std::floor(gy)) % 200;
        if (gi < 0)
            gi += 200;
        const double gf = gy - std::floor(gy),
                     g = gradient[gi] * (1 - gf) + gradient[(gi + 1) % 200] * gf;
        return smooth(a, g);
    };
    unsigned max_texture_error = 0;
    for (unsigned clock : {0u, 33u, 1023u, 5000u, 9999u, 10000u, 3599999u, 3600000u, 0xffffffffu}) {
        const auto phase = tanx_scene::phase(clock);
        CHECK(phase == tanx_scene::phase(clock % 10000));
        for (int v = -10; v < 65550; v += 251)
            for (int u = -10; u < 65550; u += 809) {
                const unsigned a = smoke.alpha(u, v, phase), b = reference(u, v, phase),
                               error = a > b ? a - b : b - a;
                if (error > max_texture_error)
                    max_texture_error = error;
                CHECK(error <= 1);
            }
    }
    CHECK(tanx_scene::phase(0) == tanx_scene::phase(10000));
    double maximum_position = 0;
    for (unsigned seed = 0; seed < 16; ++seed) {
        fake_merchant::Square p{};
        p.birth = 1;
        tanx_particles::spawn(p, seed, 0x71ae1234u);
        double x = p.x / 1024.0, y = p.y / 1024.0, vx = 0, vy = 0;
        const double radial = p.influence / 1024.0,
                     angle = (p.pan_x / 65536.0) * 2 * 3.14159265358979323846 / 30;
        for (unsigned frame = 0; frame < 450; ++frame) {
            const double length = std::hypot(x, y);
            vx += (50 + (length ? x * radial / length : 0)) / 30;
            vy += (-27 + (length ? y * radial / length : 0)) / 30;
            const double nx = x * std::cos(angle) + y * std::sin(angle) + vx / 30;
            y = y * std::cos(angle) - x * std::sin(angle) + vy / 30;
            x = nx;
            tanx_particles::step(p);
            const double error = std::hypot(p.x / 1024.0 - x, p.y / 1024.0 - y) * 320 / 1680;
            if (error > maximum_position)
                maximum_position = error;
            CHECK(error < 1.0);
        }
    }
    fake_merchant::Squares state{};
    tanx_particles::advance(state, 0, 7);
    auto entry = state;
    CHECK(state.tick == 451);
    for (unsigned i = 0; i < 40; ++i)
        CHECK(state.particles[i].active);
    tanx_particles::advance(state, 0, 7);
    CHECK(state.tick == entry.tick);
    for (unsigned i = 0; i < 40; ++i)
        CHECK(state.particles[i].x == entry.particles[i].x &&
              state.particles[i].y == entry.particles[i].y);
    tanx_particles::advance(state, 12345, 7);
    tanx_particles::advance(state, 0, 7);
    for (unsigned i = 0; i < 40; ++i)
        CHECK(state.particles[i].x == entry.particles[i].x &&
              state.particles[i].y == entry.particles[i].y);
    for (unsigned i = 40; i < 60; ++i)
        CHECK(!state.particles[i].active);
    std::printf("Tanx: %u checks; smoothstep error %u/255; full source-bilinear error %u/255; max "
                "physics LCD error %.9f; no new particle state\n",
                checks, max_alpha_error, max_texture_error, maximum_position);
}
