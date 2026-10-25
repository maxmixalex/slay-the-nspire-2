#pragma once
#include "settings_ui10_assets.hpp"
namespace spire {
constexpr unsigned Ui12ContentVersion=40;
enum Ui12Asset {
 I12_PileCount=SettingsUi10AuxiliaryCount,I12_PlayerSpeech,I12_RandomCharacter,
 I12_CharacterTransitionFirst,I12_CharacterTransitionSilent,I12_CharacterTransitionNecrobinder,
 I12_RandomBackground,
 Ui12AuxiliaryCount
};
static_assert(I12_PileCount==2154&&I12_RandomBackground==2160&&Ui12AuxiliaryCount==2161,"UI12 source image catalog");

}
