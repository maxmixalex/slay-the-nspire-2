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
 I8_MapDraw,
 I8_MapDrawGlow,
 I8_MapErase,
 I8_MapEraseGlow,
 I8_MapClear,
 I8_MapClearGlow,
 I8_MapCursorDraw,
 I8_MapCursorDrawTilted,
 I8_MapCursorErase,
 I8_MapCursorEraseTilted,
 I8_MapDrawTrail,
 I8_MapEraseTrail,
 HistoryUi8AuxiliaryCount
};
static_assert(I8_TinyCardBack==1945&&I8_ModifierDraft==1955&&I8_MapDraw==1972&&HistoryUi8AuxiliaryCount==1984,"Original history, modifier and map drawing layer order");
}
