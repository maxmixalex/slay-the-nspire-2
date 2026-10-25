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
// Optional fixed catalog: 0..3 seconds of the source Spine logo at 12 Hz.
// Existing UI40 packs stop at2161 and remain valid without an intro.
constexpr unsigned I12_IntroLogoFirst=Ui12AuxiliaryCount,IntroLogoFrames=37;
constexpr unsigned IntroLogoAuxiliaryCount=I12_IntroLogoFirst+IntroLogoFrames;
constexpr unsigned I12_PhobiaSwarm=IntroLogoAuxiliaryCount,PhobiaSwarmAuxiliaryCount=I12_PhobiaSwarm+1;
static_assert(I12_PhobiaSwarm==2198&&PhobiaSwarmAuxiliaryCount==2199,"Bounded source particle append");
constexpr unsigned I12_SleepingZEnglish=PhobiaSwarmAuxiliaryCount,I12_SleepingZJapanese=I12_SleepingZEnglish+1;
constexpr unsigned SleepingVfxAuxiliaryCount=I12_SleepingZJapanese+1;
static_assert(I12_SleepingZEnglish==2199&&I12_SleepingZJapanese==2200&&SleepingVfxAuxiliaryCount==2201,"Original localized sleeping particles");
constexpr unsigned IntroLogoHeaderString=8699,IntroLogoDescriptionString=8700;
constexpr unsigned IntroLogoOnString=8701,IntroLogoOffString=8702;
}
