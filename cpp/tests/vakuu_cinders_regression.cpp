#ifdef NDEBUG
#undef NDEBUG
#endif
#include "vakuu_cinders.hpp"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iterator>
#include <vector>
using namespace spire::tez_cinders;
static State state;
struct Picture {
    const uint8_t* pixels;
    int w, h, format;
    const uint8_t *alpha, *palette;
};
int main(int argc, char** argv) {
    assert(argc == 1 || argc == 3);
    std::vector<uint8_t> parameter(64), raw(3072);
    if (argc == 3) {
        std::ifstream a(argv[1], std::ios::binary), b(argv[2], std::ios::binary);
        parameter.assign(std::istreambuf_iterator<char>(a), {});
        raw.assign(std::istreambuf_iterator<char>(b), {});
    } else {
        // Source-free CI fixture; optional arguments retain the actual-source gate.
        const auto put = [](uint8_t* p, uint32_t v, unsigned count) {
            for (unsigned i = 0; i < count; ++i)
                p[i] = uint8_t(v >> (i * 8));
        };
        memcpy(parameter.data(), "VKC1", 4);
        const unsigned header[] = {1, 24, 59710, 59711, 7221, 0};
        for (unsigned i = 0; i < 6; ++i)
            put(parameter.data() + 4 + i * 2, header[i], 2);
        const int geometry[] = {350802, 229028, -228827, 350854, -9313, 51931};
        for (unsigned i = 0; i < 6; ++i)
            put(parameter.data() + 16 + i * 4, uint32_t(geometry[i]), 4);
        for (unsigned i = 0; i < 768; ++i)
            put(raw.data() + 4 * i, i < 256 ? i * 4096 : 1048576, 4);
    }
    assert(parameter.size() == 64 && raw.size() == 3072);
    spire::vakuu_cinders::Parameters p;
    assert(p.read(Picture{parameter.data(), 32, 1, 0, nullptr, nullptr}));
    auto invalid = parameter;
    invalid[8] ^= 1;
    assert(!spire::vakuu_cinders::Parameters{}.read(
        Picture{invalid.data(), 32, 1, 0, nullptr, nullptr}));
    invalid = parameter;
    invalid[63] = 1;
    assert(!spire::vakuu_cinders::Parameters{}.read(
        Picture{invalid.data(), 32, 1, 0, nullptr, nullptr}));
    static_assert(sizeof(State) == 56844,
                  "Vakuu mode uses existing metadata padding, no state growth");
    memset(state.particles + 24, 0xa5, (Particles - 24) * sizeof(Particle));
    const SourceCurve orbit{raw.data()};
    unsigned checks = 3, polls = 0;
    for (uint32_t clock :
         {0u, 16u, 33u, 1000u, 6000u, 6999u, 7000u, 7023u, 14000u, 140u, 0xffffffffu}) {
        constexpr uint32_t seed = 0x89123456;
        state.advance(clock, seed, orbit, [&]() { ++polls; }, NoInheritance{}, true);
        const uint32_t target = uint32_t(uint64_t(clock) * 60 / 1000) + 360;
        for (unsigned i = 0; i < 24; ++i) {
            Particle reference{};
            const uint64_t now = uint64_t(target) * 2, phase = 420u * 2 * i / 24;
            if (now >= phase) {
                const uint64_t local = now - phase;
                const unsigned age = unsigned(local % 840), cycle = unsigned(local / 840);
                birth(reference, false, hash(seed) + cycle * 24 + i);
                if (age & 1)
                    step(reference, orbit, 1);
                while (reference.age_half < age && reference.active)
                    step(reference, orbit, 2);
                if (!reference.active)
                    reference.age_half = uint16_t(age);
            }
            assert(!memcmp(&reference, &state.particles[i], sizeof(Particle)));
            ++checks;
        }
        const auto* untouched = reinterpret_cast<const uint8_t*>(state.particles + 24);
        for (unsigned i = 0; i < (Particles - 24) * sizeof(Particle); ++i) {
            assert(untouched[i] == 0xa5);
            ++checks;
        }
    }
    assert(polls == 11);
    printf("{\"checks\":%u,\"particles_advanced_per_frame\":24,\"state_bytes\":%zu,\"untouched_"
           "other_particles\":1752}\n",
           checks, sizeof(State));
}
