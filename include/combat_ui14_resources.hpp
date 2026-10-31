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
 I14_SovereignBladeCount=3,I14_SmithHammer=I14_SovereignBlade+I14_SovereignBladeCount,
 I14_SmithImpact,I14_SmithSpark,I14_EnchantSpark,CombatUi14AuxiliaryCount
};
// Full original infection sequence is15fps. Shader scenes retain their
// original time units in eight sampled phases; ringing has a2.5s period.
inline unsigned card_affliction_frame(unsigned kind,uint32_t now){
 // Entangled and Tainted repeat in40/7 seconds, not5714 milliseconds.
 // Reduce their rational clock over40 seconds before multiplying; each
 // complete window advances56 phases, so long sessions cannot drift.
 switch(kind){
 case 1:return (now/500u)%I14_AfflictionFrames;
 case 2:case 5:return ((now%40000u)*7u/5000u)%I14_AfflictionFrames;
 case 3:return (now/1250u)%I14_AfflictionFrames;
 case 4:case 7:return (now/2500u)%I14_AfflictionFrames;
 case 6:return (now%2500u)*I14_AfflictionFrames/2500u;
 default:return 0;
 }
}
}
