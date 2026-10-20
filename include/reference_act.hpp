#pragma once
#include "reference_map.hpp"

namespace spire::reference {
enum class ActKind : uint8_t { Overgrowth, Underdocks, Hive, Glory };
enum class QuotaError : uint8_t { None, InvalidInput, AmbiguousRounding, IterationLimit };
struct ActSettings {
    ActKind act = ActKind::Overgrowth;
    bool increased_elites = false;
    bool replace_treasure = false;
    bool second_boss = false;
    uint8_t variant = 0;
};
struct QuotaTrace {
    QuotaError error = QuotaError::None;
    uint32_t rest_calls = 0, unknown_calls = 0;
};
// Integer branch of Rng.NextGaussianInt, not NextGaussianDouble. Values outside
// the inclusive range are redrawn; no clamping, cached pair or alternate RNG.
bool gaussian_int(Random&, int mean, int deviation, int minimum, int maximum,
                  int& result, QuotaError& error);
unsigned act_index(ActKind); // Zero-based registry index, not the preview tab.
bool act_quotas(Random&, const ActSettings&, MapConfig&, QuotaTrace&);
// On failure returns an explicit error map; no fallback map or skipped sample.
bool generate_act_map(ReferenceMap&, uint64_t run_seed, const ActSettings&,
                      MapWorkspace&, QuotaTrace&);
// Shared bounded scratch storage for serial UI/run generation. No map retains
// pointers into the workspace, so preview and campaign do not duplicate it.
bool generate_act_map(ReferenceMap&, uint64_t run_seed, const ActSettings&,
                      QuotaTrace&);
}
