#pragma once
#include <stdint.h>

namespace spire {
// Private per-frame word tables keep source movement independent of the
// bitmap canvas. Public data contains directory identities, never geometry.
constexpr unsigned LayeredAnimationBaseAuxiliary = 5409;
constexpr int LayeredAnimationOstyDeadLoop = -2;
constexpr unsigned LayeredAnimationClipCount = 10;
constexpr unsigned LayeredAnimationHeaderWords = 10, LayeredAnimationFrameWords = 2,
                   LayeredAnimationLayerWords = 3;
constexpr unsigned LayeredAnimationMaxLayers = 16;
constexpr uint16_t LayeredAnimationMagic = 0x4652;
constexpr unsigned I20_RewardRare = LayeredAnimationBaseAuxiliary + LayeredAnimationClipCount;
constexpr unsigned I20_RewardUncommon = I20_RewardRare + 1, I20_RewardSpecial = I20_RewardRare + 2;
constexpr unsigned LayeredAnimationAuxiliaryCount = I20_RewardSpecial + 1;
inline int layered_animation_row(int clip) {
    switch (clip) {
    case 105:
        return 0;
    case 123:
        return 1;
    case 124:
        return 2;
    case 125:
        return 3;
    case 548:
        return 4;
    case 706:
        return 5;
    case 707:
        return 6;
    case 1263:
        return 7;
    case 1626:
        return 8;
    case LayeredAnimationOstyDeadLoop:
        return 9;
    default:
        return -1;
    }
}
} // namespace spire
