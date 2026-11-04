#pragma once
#include <stdint.h>

namespace spire {
// Optional calculator timing for independently rendered character-selection
// layers. These identifiers do not enter canonical game state.
enum class RuntimeRenderPart : uint8_t {
    RandomWater,
    RandomLight,
    RandomDots,
    CharacterInfo,
    CharacterPortraits,
    CharacterControls,
    CharacterAscension,
    Count
};
#ifdef SPIRE_NEOW_LAYER_PROFILE
// Development-only reuse of the seven existing timing slots. No new pool or
// timing arrays are introduced, and ordinary builds contain no Neow scopes.
struct RuntimeScopedRenderPart {
    void (*callback)(RuntimeRenderPart, bool);
    RuntimeRenderPart part;
    RuntimeScopedRenderPart(void (*cb)(RuntimeRenderPart, bool), RuntimeRenderPart p,
                            bool active = true)
        : callback(active ? cb : nullptr), part(p) {
        if (callback)
            callback(part, true);
    }
    ~RuntimeScopedRenderPart() {
        if (callback)
            callback(part, false);
    }
};
#endif
static_assert(unsigned(RuntimeRenderPart::Count) <= 8,
              "Render scopes must fit the diagnostic active mask");
} // namespace spire
