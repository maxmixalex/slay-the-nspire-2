#pragma once
#include "world_assets.hpp"

namespace spire {
// NSP2/16 appends component skins without changing the historical v14/v15
// catalog, animation frame ids, or any saved game data.
enum NativeUiAsset {
    U_HealthBarBackground=WorldAuxiliaryCount,
    U_HealthBarStroke,
    U_HealthBarMask,
    U_HealthBarFill,
    U_HealthBarInfinity,
    U_HealthBarBlock,
    U_HoverTip,
    U_HoverTipDebuff,
    U_PauseButton,
    U_PauseButtonHover,
    U_EndTurnNative,
    U_EndTurnHover,
    U_EndTurnGlow,
    NativeUiAuxiliaryCount
};
}
