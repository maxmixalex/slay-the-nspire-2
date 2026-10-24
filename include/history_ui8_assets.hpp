#pragma once
#include "hud_ui7_assets.hpp"
namespace spire {
constexpr unsigned HistoryUi8ContentVersion=28;
enum HistoryUi8Asset {
 I8_TinyCardBack=HudUi7AuxiliaryCount,
 I8_TinyCardDescription,
 I8_TinyCardAttack,
 I8_TinyCardAttackShadow,
 I8_TinyCardSkill,
 I8_TinyCardSkillShadow,
 I8_TinyCardPower,
 I8_TinyCardPowerShadow,
 I8_TinyCardBanner,
 I8_TinyCardBannerShadow,
 I8_ModifierDraft,
 I8_ModifierSealedDeck,
 I8_ModifierHoarder,
 I8_ModifierSpecialized,
 I8_ModifierInsanity,
 I8_ModifierAllStar,
 I8_ModifierFlight,
 I8_ModifierVintage,
 I8_ModifierCharacterCards,
 I8_ModifierDeadlyEvents,
 I8_ModifierCursedRun,
 I8_ModifierBigGameHunter,
 I8_ModifierMidas,
 I8_ModifierMurderous,
 I8_ModifierNightTerrors,
 I8_ModifierTerminal,
 I8_EmptyOrb,
 HistoryUi8AuxiliaryCount
};
static_assert(I8_TinyCardBack==1945&&I8_ModifierDraft==1955&&HistoryUi8AuxiliaryCount==1972,"Original history and modifier layer order");
}
