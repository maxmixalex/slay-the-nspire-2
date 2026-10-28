#pragma once
#include <stdint.h>
#include "reference_viewport.hpp"
namespace spire::character_select_layout {
using reference_viewport::project;
// Five playable portraits and the source Random button share a centered row.
// The actual 4:3 viewport is 1680x1260; the row begins at 1260-209.
constexpr int PortraitTop=201,PortraitHeight=35,PortraitWidth=30;
constexpr int portrait_x(unsigned index){return 55+int(index)*36;}
constexpr int portrait_center(unsigned index){return portrait_x(index)+PortraitWidth/2;}
// InfoPanel/VBox has center alignment and fixed Name / HpGoldSpacer /
// Relic minimum heights. Its background is independent of localized text.
constexpr int InfoX=16,InfoY=22,InfoWidth=177,InfoHeight=141;
constexpr int NameHeight=25,StatsHeight=17,RelicHeight=42;
constexpr int InfoPanelX=3,InfoPanelY=13,InfoPanelWidth=203,InfoPanelHeight=160;
constexpr int StatsOffset=-1,StatsIcon=11,StatsGap=14,StatsLabelGap=3;
constexpr int RelicIcon=18,RelicIconY=2,RelicTextX=22,RelicTextWidth=155;
constexpr int RelicTitleY=0,RelicDescriptionY=11;
constexpr int info_top(int description_height){
    // Godot containers expand to their combined minimum size; center
    // alignment must never shift a longer locale above the VBox origin.
    (void)description_height;return InfoY;
}
constexpr int AscensionX=73,AscensionY=172,AscensionWidth=174,AscensionHeight=29;
constexpr unsigned FirstClip=1705;
// UI12's appended source catalog. Keeping these here also lets older source
// checkpoints compile while their optional UI12 resource pack is unavailable.
constexpr unsigned SourceVersion=40;
constexpr uint32_t Durations[5]={5333,11667,13333,6667,8000};
inline unsigned frame(unsigned character,uint32_t elapsed,unsigned count){
 return character<5&&count?unsigned(uint64_t(elapsed%Durations[character])*count/Durations[character]):0;
}
}
