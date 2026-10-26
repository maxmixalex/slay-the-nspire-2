#pragma once
#include <stdint.h>
namespace spire {
enum CombatUi14Image : unsigned {
 I14_AncientBorder=5156,I14_AncientTextAttack,I14_AncientTextSkill,I14_AncientTextPower,
 I14_AncientBanner,I14_AncientHighlight,I14_AncientFlame,
 I14_AncientFlameCount=10,I14_Infection=I14_AncientFlame+I14_AncientFlameCount,
 I14_InfectionCount=30,I14_Afflictions=I14_Infection+I14_InfectionCount,
 I14_AfflictionFrames=8,I14_AfflictionCount=7,
 I14_SovereignBlade=I14_Afflictions+I14_AfflictionFrames*I14_AfflictionCount,
 I14_SovereignBladeCount=3,CombatUi14AuxiliaryCount=I14_SovereignBlade+I14_SovereignBladeCount
};
// Full original infection sequence is15fps. Shader scenes retain their
// original time units in eight sampled phases; ringing has a2.5s period.
inline unsigned card_affliction_frame(unsigned kind,uint32_t now){
 const uint32_t periods[]={4000,5714,10000,20000,5714,2500,20000};
 return kind>=1&&kind<=7?unsigned((uint64_t(now%periods[kind-1])*I14_AfflictionFrames)/periods[kind-1]):0;
}
}
