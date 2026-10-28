#pragma once
#include "native_ui_assets.hpp"

namespace spire {
// NSP2/17 appends original card framing and top-panel component art. Keep
// NativeUiAuxiliaryCount unchanged so historical NSP2/16 packs remain valid.
enum CardHudAsset {
    U_CardBorderCommonAttack=NativeUiAuxiliaryCount,
    U_CardBorderCommonSkill,
    U_CardBorderCommonPower,
    U_CardPlaqueCommon,
    U_CardBannerCommon,
    U_CardBorderUncommonAttack,
    U_CardBorderUncommonSkill,
    U_CardBorderUncommonPower,
    U_CardPlaqueUncommon,
    U_CardBannerUncommon,
    U_CardBorderRareAttack,
    U_CardBorderRareSkill,
    U_CardBorderRarePower,
    U_CardPlaqueRare,
    U_CardBannerRare,
    U_CardBorderCurseAttack,
    U_CardBorderCurseSkill,
    U_CardBorderCursePower,
    U_CardPlaqueCurse,
    U_CardBannerCurse,
    U_CardBorderStatusAttack,
    U_CardBorderStatusSkill,
    U_CardBorderStatusPower,
    U_CardPlaqueStatus,
    U_CardBannerStatus,
    U_CardBorderEventAttack,
    U_CardBorderEventSkill,
    U_CardBorderEventPower,
    U_CardPlaqueEvent,
    U_CardBannerEvent,
    U_CardBorderQuestAttack,
    U_CardBorderQuestSkill,
    U_CardBorderQuestPower,
    U_CardPlaqueQuest,
    U_CardBannerQuest,
    U_CardBorderAncientAttack,
    U_CardBorderAncientSkill,
    U_CardBorderAncientPower,
    U_CardPlaqueAncient,
    U_CardBannerAncient,
    U_TopBarBackdrop,
    U_TopBarIronclad,
    U_TopBarSilent,
    U_TopBarDefect,
    U_TopBarRegent,
    U_TopBarNecrobinder,
    U_PotionPlaceholder,
    U_TopBarAscension,
    CardHudAuxiliaryCount
};
static_assert(CardHudAuxiliaryCount==1236,"Card/HUD asset directory must match config/card-hud-assets.json");
}
