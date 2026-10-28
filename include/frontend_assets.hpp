#pragma once
#include "title_assets.hpp"

namespace spire {
// NSP2/22 retains the v21 codecs, catalog indices and all earlier payloads.
// Original settings, potion-popup and star-counter textures, in this order.
constexpr unsigned FrontendContentVersion=22;
// First unused v21 locale slots. Existing sorted UI bindings end at 8102;
// extending that sorted table would silently reassign historical indices.
constexpr unsigned FrontendSettingsGeneralString=8103,FrontendSettingsInputString=8104;
enum FrontendAsset {
    F_SettingsTab=TitleEffectsAuxiliaryCount,
    F_SettingsTabStroke,
    F_SettingsTicked,
    F_SettingsUnticked,
    F_PotionPopup,
    F_PotionPopupTop,
    F_PotionPopupBottom,
    F_StarCounterBase,
    F_StarCounterLayer2,
    F_StarCounterLayer3,
    FrontendAuxiliaryCount
};
static_assert(F_StarCounterBase==1272&&FrontendAuxiliaryCount==1275,"Frontend catalog must match patch_frontend_assets.py");
}
