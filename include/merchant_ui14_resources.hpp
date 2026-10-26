#pragma once
#include "feedback_assets.hpp"
#include "combat_ui14_resources.hpp"

namespace spire {
// Exact invariant pixels from the existing ten merchant clips. Their moving
// planes keep the original image IDs, geometry, pivots and frame clocks.
constexpr unsigned I14_MerchantFixed=CombatUi14AuxiliaryCount;
constexpr unsigned MerchantUi14AuxiliaryCount=I14_MerchantFixed+10;
constexpr int merchant_fixed_asset(int clip){
 return clip>=AnimMerchantRoomBottom&&clip<MerchantEffectsAnimationKinds?
  int(I14_MerchantFixed)+clip-AnimMerchantRoomBottom:-1;
}
static_assert(I14_MerchantFixed==5265&&MerchantUi14AuxiliaryCount==5275,
              "Merchant exact layer catalog must match merchant_ui14_assets.py");
static_assert(MerchantEffectsAnimationKinds-AnimMerchantRoomBottom==10,
              "All ten original merchant clips retain their registration");
}
