#pragma once
#include "screen_shake.hpp"
namespace spire {
struct ArchitectShakeTimeline {
 unsigned hits=0,lead=0,windup=0,architect_at=0,complete=0;
 unsigned hit_at(unsigned i)const{return lead+(i+1)*windup+i*100u;}
};
inline ArchitectShakeTimeline source_architect_shake(Character character,unsigned dialogue,bool start,bool fast){
 // TheArchitect's authored player hit list has five entries, except the
 // Silent and Necrobinder's four. Only Silent attacks before dialogue; her
 // later two dialogues leave the ending attack to the Architect alone.
 ArchitectShakeTimeline p;
 p.hits=start?(character==Character::Silent?4u:0u):character==Character::Silent&&dialogue>=2?0u:
  character==Character::Silent||character==Character::Necrobinder?4u:5u;
 if(start&&!p.hits)return p;
 p.lead=start?750u:0u;p.windup=fast?50u:100u;
 if(p.hits)p.complete=p.hit_at(p.hits-1)+2000u;
 // Cmd.Wait stays unscaled. Only CreatureCmd.TriggerAnim uses Fast's
 // min(wait*.5,.25), including the Architect's FireBurst wind-up.
 if(!start){p.architect_at=p.complete+(fast?250u:500u);p.complete=p.architect_at+500u;}
 return p;
}
struct SourceImpactShake {uint16_t delay=0,duration=300;uint8_t strength=0;};
inline SourceImpactShake source_card_impact_shake(CardKind kind){
 // These are the explicit source VFX families, not an inference from card
 // damage. A blocked hit still constructs its authored hit VFX.
 using S=ScreenShakePresentation;
 struct Row {const char*key;uint16_t delay,duration;uint8_t strength;};
 static const Row rows[]={
  {"PERFECTED_STRIKE",100,300,S::Strong},{"SOVEREIGN_BLADE",100,300,S::Strong},
  {"DAGGER_SPRAY",100,300,S::Weak},{"DAGGER_THROW",100,300,S::Weak},{"SHIV",150,300,S::Weak},
  {"CLAW",100,300,S::VeryWeak},{"MAUL",100,300,S::VeryWeak},{"RIP_AND_TEAR",100,300,S::VeryWeak},
  {"ALL_FOR_ONE",200,300,S::Strong},{"BLUDGEON",200,300,S::Strong},{"BREAK",200,300,S::Strong},
  {"CRASH_LANDING",200,300,S::Strong},{"CRUSH_UNDER",200,300,S::Strong},{"MANGLE",200,300,S::Strong},
  {"MIDNIGHT",200,300,S::Strong},{"PACTS_END",200,300,S::Strong},{"STOMP",200,300,S::Strong},
  {"UNRELENTING",200,300,S::Strong},{"UPPERCUT",200,300,S::Strong},
  {"ASTRAL_PULSE",0,300,S::Weak},{"CELESTIAL_MIGHT",0,300,S::Weak},{"CRESCENT_SPEAR",0,300,S::Weak},
  {"DYING_STAR",0,300,S::Weak},{"FALLING_STAR",0,300,S::Weak},{"MAKE_IT_SO",0,300,S::Weak},
  {"RADIATE",0,300,S::Weak},{"SEVEN_STARS",0,300,S::Weak},{"SHINING_STRIKE",0,300,S::Weak},{"STARDUST",0,300,S::Weak},
  {"CINDER",0,800,S::Weak},{"GRAND_FINALE",0,800,S::Strong},{"SWEEPING_BEAM",0,800,S::Medium},
  {"COMET",200,300,S::Medium},{"GUIDING_STAR",200,300,S::Medium},
  {"BOMBARDMENT",200,800,S::Strong},{"END_OF_DAYS",200,800,S::Strong},{"METEOR_STRIKE",200,800,S::Strong},
  {"MINION_DIVE_BOMB",0,300,S::Weak}
 };
 if(kind>=CardKinds)return {};const char*key=definitions[kind].key;
 for(const auto&r:rows){const char*a=key,*b=r.key;while(*a&&*a==*b){++a;++b;}if(!*a&&!*b)return {r.delay,r.duration,r.strength};}
 return {};
}
}
