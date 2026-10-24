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
 HistoryUi8AuxiliaryCount
};
static_assert(I8_TinyCardBack==1945&&HistoryUi8AuxiliaryCount==1955,"Original tiny-card layer order");
}
