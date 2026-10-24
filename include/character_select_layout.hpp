#pragma once
#include <stdint.h>
#include "reference_viewport.hpp"
namespace spire::character_select_layout {
using reference_viewport::project;
// Five 100px portraits plus four 16px gaps center within the source HBox.
// The actual 4:3 viewport is 1680x1260; the row begins at 1260-209.
constexpr int PortraitTop=project(1051),PortraitHeight=project(148),PortraitWidth=project(100);
constexpr int portrait_x(unsigned index){return project(558+int(index)*116);}
constexpr int portrait_center(unsigned index){return project(608+int(index)*116);}
// InfoPanel/VBox has center alignment and fixed Name / HpGoldSpacer /
// Relic minimum heights. Its background is independent of localized text.
constexpr int InfoX=project(163),InfoY=project(414),InfoWidth=project(575),InfoHeight=project(380);
constexpr int NameHeight=project(140),StatsHeight=project(20),RelicHeight=project(100);
constexpr int InfoPanelX=project(89),InfoPanelY=project(411),InfoPanelWidth=project(700),InfoPanelHeight=project(402);
constexpr int StatsOffset=project(-26),StatsIcon=project(40),StatsGap=project(48),StatsLabelGap=project(4);
constexpr int RelicIcon=project(64),RelicIconY=project(8),RelicTextX=project(72),RelicTextWidth=project(420);
constexpr int RelicTitleY=project(12),RelicDescriptionY=project(44);
constexpr int info_top(int description_height){return InfoY+(InfoHeight-NameHeight-StatsHeight-description_height-RelicHeight)/2;}
constexpr int AscensionX=project(539),AscensionY=project(918),AscensionWidth=project(600),AscensionHeight=project(118);
constexpr unsigned FirstClip=1705;
constexpr uint32_t Durations[5]={5333,11667,13333,6667,8000};
inline unsigned frame(unsigned character,uint32_t elapsed,unsigned count){
 return character<5&&count?unsigned(uint64_t(elapsed%Durations[character])*count/Durations[character]):0;
}
}
