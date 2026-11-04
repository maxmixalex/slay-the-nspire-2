#pragma once
#include "fake_merchant_presentation.hpp"

namespace spire::tanx_particles {
using fake_merchant::Square;
using fake_merchant::Squares;
constexpr unsigned Count = 40, Life = 450, Rate = 30;
constexpr unsigned tick(uint32_t elapsed) {
    return Life + unsigned(uint64_t(elapsed) * Rate / 1000);
}
inline void spawn(Square& p, unsigned index, uint32_t seed) {
    const auto hash = fake_merchant::hash;
    const uint32_t h = hash(seed ^ index * 0x9e3779b9u ^ (p.birth / Life) * 0x85ebca6bu);
    p.seed = h;
    p.active = true;
    p.age = 0;
    p.vx = p.vy = 0;
    p.x = int(hash(h) % 1638401u) - 819200;
    p.y = int(hash(h + 1) % 1024001u) - 512000;
    p.scale = uint16_t(1024 + hash(h + 2) % (index < 20 ? 3073u : 1025u));
    p.angular = int16_t(uint64_t(hash(h + 3) % 65536u) * 4023 / 65535); // source0..353.6deg
    p.influence = uint16_t(hash(h + 4) % 1025u); // radial acceleration0..1 Q10
    p.pan_x = int16_t(hash(h + 5) % 1967u);      // orbit turns/s0...03 Q16
}
inline void step(Square& p) {
    const unsigned distance =
        fake_merchant::root(uint64_t(int64_t(p.x) * p.x + int64_t(p.y) * p.y));
    int ax = 50 * 1024, ay = -27 * 1024;
    if (distance) {
        ax += int(int64_t(p.x) * p.influence / distance);
        ay += int(int64_t(p.y) * p.influence / distance);
    }
    p.vx += ax / int(Rate);
    p.vy += ay / int(Rate);
    // Source CPU particles rotate the position clockwise before adding velocity.
    // Taylor sin/cos at |angle|<=.006284 retains much finer resolution than
    // an LCD angle table; int64 intermediates stay within their proven bounds.
    const int64_t angle = int64_t(p.pan_x) * 1686629713 / (65536 * 30); // radians Q28
    const int64_t square = (angle * angle) >> 28;
    const int64_t sn = angle - ((square * angle) >> 28) / 6, cs = (int64_t(1) << 28) - square / 2;
    const int32_t x = p.x, y = p.y;
    p.x = int32_t((int64_t(x) * cs + int64_t(y) * sn) / (int64_t(1) << 28)) + p.vx / int(Rate);
    p.y = int32_t((int64_t(y) * cs - int64_t(x) * sn) / (int64_t(1) << 28)) + p.vy / int(Rate);
    ++p.age;
}
inline void reset(Squares& s, unsigned next, uint32_t seed) {
    s.initialized = true;
    s.seed = seed;
    s.tick = next > Life ? next - Life : 0;
    for (unsigned i = 0; i < Squares::Count; ++i) {
        auto& p = s.particles[i];
        p = {};
        if (i >= Count)
            continue;
        const unsigned phase = (i % 20) * Life / 20 + 1;
        p.birth = phase + (s.tick > phase ? (s.tick - phase + Life - 1) / Life : 0) * Life;
    }
}
inline void advance(Squares& s, uint32_t elapsed, uint32_t seed) {
    const unsigned next = tick(elapsed);
    if (!s.initialized || s.seed != seed || next + 1 < s.tick ||
        (next >= s.tick && next - s.tick > Life))
        reset(s, next, seed);
    for (; s.tick <= next; ++s.tick)
        for (unsigned i = 0; i < Count; ++i) {
            auto& p = s.particles[i];
            if (s.tick == p.birth) {
                spawn(p, i, seed);
                p.birth += Life;
            } else if (p.active)
                step(p);
        }
}
constexpr fake_merchant::ColorKey GrainColor[] = {
    fake_merchant::color_key(0, .913725, .313726, .227451, 0),
    fake_merchant::color_key(.178337, .913725, .313726, .227451, 1),
    fake_merchant::color_key(.573304, .827451, .411765, .258824, 1),
    fake_merchant::color_key(1, 0, 0, 0, 1)};
constexpr fake_merchant::ColorKey RiceColor[] = {
    fake_merchant::color_key(0, .913725, .313726, .227451, 0),
    fake_merchant::color_key(.178337, 0, 0, 0, 1), fake_merchant::color_key(1, 0, 0, 0, 1)};
} // namespace spire::tanx_particles
