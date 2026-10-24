#pragma once
#include <stdint.h>
namespace spire::character_select_layout {
// Original 1440x1080 4:3 control anchors, uniformly projected by 2/9.
constexpr int PortraitTop=194,PortraitHeight=33,PortraitWidth=22;
constexpr int portrait_x(unsigned index){return int((438+index*116)*2+4)/9;}
constexpr int portrait_center(unsigned index){return int((488+index*116)*2+4)/9;}
constexpr int InfoX=10,InfoWidth=128,InfoBottom=158;
constexpr int AscensionX=93,AscensionY=165,AscensionWidth=133,AscensionHeight=26;
constexpr unsigned FirstClip=1705;
constexpr uint32_t Durations[5]={5333,11667,13333,6667,8000};
inline unsigned frame(unsigned character,uint32_t elapsed,unsigned count){
 return character<5&&count?unsigned(uint64_t(elapsed%Durations[character])*count/Durations[character]):0;
}
}
