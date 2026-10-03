#include "reference_act.hpp"
#include "reference_math.hpp"

namespace spire::reference {
bool gaussian_int(Random& rng, int mean, int deviation, int minimum, int maximum,
                  int& result, QuotaError& error) {
    error = QuotaError::None;
    // This sampler is intentionally bounded to map-quota use. It does not
    // claim to implement arbitrary .NET Int32 gaussian argument edge cases.
    if (!rng.good || minimum > maximum || mean < minimum || mean > maximum ||
        deviation < 0 || deviation > 1000 || minimum < -1000 || maximum > 1000) {
        error = QuotaError::InvalidInput; return false;
    }
    using math::Real;
    constexpr Real one = Real::raw(UINT64_C(0x3ff0000000000000));
    constexpr Real minus_two = Real::raw(UINT64_C(0xc000000000000000));
    constexpr Real tau = Real::raw(UINT64_C(0x401921fb54442d18));
    for (unsigned attempt = 0; attempt < 4096; ++attempt) {
        Real u1 = one - Real::fraction53(rng.next() >> 11);
        Real u2 = one - Real::fraction53(rng.next() >> 11);
        Real normal = math::square_root(minus_two * math::logarithm(u1)) * math::sine_cycle(tau * u2);
        Real value = Real::integer(mean) + Real::integer(deviation) * normal;
        if (math::near_half_integer(value)) {
            error = QuotaError::AmbiguousRounding; return false;
        }
        int32_t rounded = 0;
        if (!math::round_even_int(value, rounded)) { error = QuotaError::InvalidInput; return false; }
        if (rounded >= minimum && rounded <= maximum) { result = rounded; return true; }
    }
    error = QuotaError::IterationLimit; return false;
}
unsigned act_index(ActKind act) {
    return act == ActKind::Glory ? 2 : act == ActKind::Hive ? 1 : 0;
}
bool act_quotas(Random& rng, const ActSettings& settings, MapConfig& out, QuotaTrace& trace) {
    trace = QuotaTrace{};
    if (!rng.good || unsigned(settings.act) > unsigned(ActKind::Glory)) {
        trace.error = QuotaError::InvalidInput; return false;
    }
    MapConfig result{};
    result.rows = uint8_t((settings.act == ActKind::Glory ? 14 : settings.act == ActKind::Hive ? 15 : 16) -
                          (settings.multiplayer ? 1 : 0));
    result.shops = 3;
    result.elites = settings.increased_elites ? 8 : 5;
    result.replace_treasure = settings.replace_treasure;
    result.second_boss = settings.second_boss;
    uint32_t begin = rng.calls; int rests = 0, unknowns = 0;
    if (settings.act == ActKind::Glory) rests = rng.range(5, 7);
    else if (!gaussian_int(rng, settings.act == ActKind::Hive ? 6 : 7, 1, 6, 7, rests, trace.error)) return false;
    trace.rest_calls = rng.calls - begin;
    begin = rng.calls;
    if (!gaussian_int(rng, 12, 1, 10, 14, unknowns, trace.error)) return false;
    trace.unknown_calls = rng.calls - begin;
    if (settings.act == ActKind::Glory || settings.act == ActKind::Hive) --unknowns;
    result.rests = uint8_t(rests); result.unknowns = uint8_t(unknowns);
    out = result; return true;
}
bool generate_act_map(ReferenceMap& out, uint64_t run_seed, const ActSettings& settings,
                      MapWorkspace& scratch, QuotaTrace& trace) {
    char stream[] = "act_1_map"; stream[4] = char('1' + act_index(settings.act));
    Random rng; rng.named(run_seed, stream);
    MapConfig config;
    if (!act_quotas(rng, settings, config, trace)) {
        out = ReferenceMap{}; out.error = MapError::Configuration; return false;
    }
    return generate_map(out, rng, config, scratch);
}
}
