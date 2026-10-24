#pragma once
#include <cstdint>
#include "history_ui8_assets.hpp"
namespace spire {
// Original non-looping player skeletons are appended after the complete
// phobia catalog. Existing v28 packs retain their own directory contract.
constexpr unsigned CharacterDeathContentVersion=29;
constexpr unsigned CharacterDeathAnimationFirst=PhobiaAnimationKinds;
constexpr unsigned MerchantDeathAnimationFirst=CharacterDeathAnimationFirst+5;
constexpr unsigned DefectDeathParticles=CharacterDeathAnimationFirst+10;
constexpr unsigned RegentDeathParticles=CharacterDeathAnimationFirst+11;
constexpr unsigned CharacterDeathAnimationKinds=CharacterDeathAnimationFirst+12;
constexpr unsigned I9_RunTimerIcon=HistoryUi8AuxiliaryCount;
constexpr unsigned CharacterDeathAuxiliaryCount=HistoryUi8AuxiliaryCount+1;
constexpr unsigned CharacterDeathDurationMs[5]={2333,1867,2233,4167,2333};
constexpr unsigned CharacterDeathFrameCount[5]={71,57,68,126,71};
inline unsigned character_death_frame(unsigned character,uint32_t milliseconds){
 if(character>=5)character=0;
 const unsigned end=CharacterDeathFrameCount[character]-1,duration=CharacterDeathDurationMs[character];
 return milliseconds>=duration?end:milliseconds*end/duration;
}
}
