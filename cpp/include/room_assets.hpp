#pragma once
#include "interface_assets.hpp"
namespace spire {
constexpr unsigned RoomContentVersion = 24;
enum RoomAsset {
    I7_MerchantHandStart = BaseUiAuxiliaryCount,
    I7_MerchantHandLast = I7_MerchantHandStart + 46,
    I7_FakeMerchantHandStart,
    I7_FakeMerchantHandLast = I7_FakeMerchantHandStart + 46,
    I7_MerchantDialogue,
    I7_MerchantRemoval01,
    I7_MerchantRemoval02,
    I7_MerchantRemoval04,
    I7_AncientDialogue,
    I7_AncientDialogueTail,
    I7_AncientSpeakerStart,
    I7_AncientSpeakerLast = I7_AncientSpeakerStart + 7,
    RoomAuxiliaryCount
};
constexpr unsigned MerchantHandSamples = 47;
constexpr int MerchantHandPivotX = 160, MerchantHandPivotY = 230;
static_assert(I7_MerchantHandStart == 1320 && RoomAuxiliaryCount == 1428,
              "legacy room asset order");
} // namespace spire
