#pragma once
#include <stdint.h>

namespace spire {
// Optional calculator timing for independently rendered character-selection
// layers. These identifiers do not enter canonical game state.
enum class RuntimeRenderPart : uint8_t {
    RandomWater,RandomLight,RandomDots,
    CharacterInfo,CharacterPortraits,CharacterControls,CharacterAscension,Count
};
static_assert(unsigned(RuntimeRenderPart::Count)<=8,"Render scopes must fit the diagnostic active mask");
}
