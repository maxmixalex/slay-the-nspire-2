#pragma once
#include "card_hud_assets.hpp"
#include "assets.hpp"

namespace spire {
// NSP2/18 retains every v17 directory index. Layer order follows each pinned
// energy_counter.tscn; Necrobinder's unused two slots are transparent.
constexpr unsigned FeedbackContentVersion=18;
enum FeedbackAsset {
    U_EnergyLayers=CardHudAuxiliaryCount,
    U_MerchantBackdrop=U_EnergyLayers+25,
    FeedbackAuxiliaryCount
};
enum FeedbackAnimation {
    AnimMerchantRoomBottom=AnimationKinds,
    AnimMerchantRoomTop,
    AnimMerchantIronclad,
    AnimMerchantSilent,
    AnimMerchantDefect,
    AnimMerchantRegent,
    AnimMerchantNecrobinder,
    FeedbackAnimationKinds
};
static_assert(FeedbackAuxiliaryCount==1262,"Feedback image directory must match patch_feedback_assets.py");
static_assert(FeedbackAnimationKinds==1702,"Feedback clip directory must match patch_feedback_assets.py");
// NSP2/19 is append-only. Keep the exact v18 counts above for old installs.
constexpr unsigned MerchantEffectsContentVersion=19;
constexpr unsigned MerchantEffectsAuxiliaryCount=FeedbackAuxiliaryCount;
enum MerchantEffectsAnimation {
    AnimMerchantRoomFocused=FeedbackAnimationKinds,
    AnimMerchantFlamesBehind,
    AnimMerchantFlamesFront,
    MerchantEffectsAnimationKinds
};
constexpr unsigned MerchantIdleDurationMs=6000;
static_assert(MerchantEffectsAnimationKinds==1705,"Merchant effects catalog must match patch_merchant_effects.py");
}
