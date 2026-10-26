#pragma once
#include <stdint.h>

namespace spire {
// Optional calculator timing for the three independently rendered Random
// background layers. These identifiers do not enter canonical game state.
enum class RuntimeRenderPart : uint8_t { RandomWater,RandomLight,RandomDots,Count };
}
