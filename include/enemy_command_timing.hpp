#pragma once
#include "core.hpp"

namespace spire {
// Pinned v0.111.0 command boundaries, in milliseconds. Skeletons continue
// independently after a command has returned. This table covers only the
// named base moves; it does not infer waits from an animation's duration.
struct EnemyCommandTiming {
 enum Effect : uint8_t {None,Attack,Power,GeneratedCard};
 Effect effect=None;
 unsigned impact_ms=0,total_ms=0;
};
inline EnemyCommandTiming enemy_command_timing(EnemyId kind,int move,bool fast){
 if(kind==EnemyId::LeafSlimeS||kind==EnemyId::LeafSlimeM){
  if(move==0)return {EnemyCommandTiming::Attack,fast?75u:150u,fast?175u:350u};
  if(move==1){
   // TriggerAnim's Fast wait caps at 250 ms. Cards generated into Discard
   // have no preceding pile tween; AddToCombatAndPreview then waits 1 s
   // in both Normal and Fast, independently of its 1.2 s preview lifetime.
   const unsigned impact=fast?250u:kind==EnemyId::LeafSlimeS?500u:1000u;
   return {EnemyCommandTiming::GeneratedCard,impact,impact+1000u};
  }
 }else if(kind==EnemyId::DampCultist){
  // Incantation: TriggerAnim(450), explicit dialogue wait (500/250),
  // visible Ritual, then PowerCmd's post-application wait (250/100).
  if(move==0)return {EnemyCommandTiming::Power,fast?475u:950u,fast?575u:1200u};
  if(move==1)return {EnemyCommandTiming::Attack,fast?100u:200u,fast?200u:400u};
 }
 return {};
}
}
