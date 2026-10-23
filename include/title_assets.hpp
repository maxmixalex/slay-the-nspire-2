#pragma once
#include "feedback_assets.hpp"

namespace spire {
// NSP2/21 keeps the v20 codec and every existing image/clip/auxiliary index.
// Only these three references are appended; the old title logo stays image 4.
// Keep the old flattened background for the low-cost collection backdrop.
constexpr unsigned TitleEffectsContentVersion=21;
enum TitleEffectsAsset {
    U_TitleBackdrop=MerchantEffectsAuxiliaryCount,
    U_TitleForeground,
    U_TitleParticles,
    TitleEffectsAuxiliaryCount
};
static_assert(TitleEffectsAuxiliaryCount==1265,"Title directory must match title_assets.py");
}
