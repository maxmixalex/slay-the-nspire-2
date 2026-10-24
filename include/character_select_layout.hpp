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
constexpr int InfoX=project(163),InfoWidth=project(575),InfoBottom=project(869);
constexpr int InfoPanelX=project(89),InfoPanelWidth=project(700);
constexpr int AscensionX=project(539),AscensionY=project(918),AscensionWidth=project(600),AscensionHeight=project(118);
constexpr unsigned FirstClip=1705;
constexpr uint32_t Durations[5]={5333,11667,13333,6667,8000};
inline unsigned frame(unsigned character,uint32_t elapsed,unsigned count){
 return character<5&&count?unsigned(uint64_t(elapsed%Durations[character])*count/Durations[character]):0;
}
}
