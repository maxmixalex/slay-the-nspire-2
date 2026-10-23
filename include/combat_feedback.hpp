#pragma once
#include "core.hpp"
#include "health_bar.hpp"

namespace spire {
// Only the ten displayed instance IDs travel with a hand change. Cards keep
// their ordinary combat ownership; this never copies the canonical State or
// allocates another MaxCards pile. Same-clock draws share one final sample.
struct CombatHand {
 static constexpr unsigned Capacity=32;
 struct Cards {uint16_t ids[MaxHand]{};uint16_t count=0;};
 struct Cue {Cards cards{};uint32_t at=0;};
 Cards shown{},observed{};
 Cue cues[Capacity]{};
 uint8_t head=0,count=0;
 bool overflow=false;
 static Cards capture(const Pile&p){
  Cards cards;cards.count=p.count<MaxHand?p.count:MaxHand;
  for(unsigned i=0;i<cards.count;++i)cards.ids[i]=p.ids[i];return cards;
 }
 static bool same(const Cards&a,const Cards&b){
  if(a.count!=b.count)return false;
  for(unsigned i=0;i<a.count;++i)if(a.ids[i]!=b.ids[i])return false;return true;
 }
 void begin(const Pile&p){shown=observed=capture(p);head=count=0;overflow=false;}
 void append(const Pile&p,uint32_t at){
  const Cards cards=capture(p);if(same(observed,cards))return;observed=cards;
  if(overflow)return;
  if(count>head&&cues[count-1].at==at){cues[count-1].cards=cards;return;}
  if(count==Capacity&&head){for(unsigned i=head;i<count;++i)cues[i-head]=cues[i];count-=head;head=0;}
  if(count==Capacity){overflow=true;return;}
  cues[count++]={cards,at};
 }
 void sync(uint32_t now){
  while(head<count&&int32_t(now-cues[head].at)>=0)shown=cues[head++].cards;
  if(head==count)head=count=0;
 }
 void finish(){shown=observed;head=count=0;}
};
static_assert(sizeof(CombatHand)<=1024,"Hand playback stores instance IDs, not full card piles");
// This bounded projection belongs to App, never State/SaveData. The final
// canonical values remain available even when an unusually long chain fills
// the queue; playback then converges at the end of that side's presentation.
struct CombatFeedback {
 static constexpr unsigned Actors=MaxEnemies+3,Energy=MaxEnemies+2,Capacity=128;
 struct Cue {HealthBarValues value{};uint32_t identity=0,at=0;uint8_t actor=0;};
 HealthBarValues shown[Actors]{},observed[Actors]{};
 uint32_t identity[Actors]{},observed_identity[Actors]{};
 EnemyIntent intents[MaxEnemies]{};
 uint32_t hide_at[MaxEnemies]{};
 Cue cues[Capacity]{};
 CombatHand hand{};
 uint32_t seed=0,effect_at=0,move_at=0,move_end=0,end_at=0,choice_at=0;
 int floor=-1,combat=-1,encounter=-1,move_slot=-1,move=-1;
 uint16_t head=0,count=0;
 uint8_t hidden=0;
 bool active=false,finished=false,overflow=false,choice_pending=false;
 void reset(){active=finished=overflow=choice_pending=false;head=count=0;hidden=0;move_slot=move=-1;}
 static bool same(const HealthBarValues&a,const HealthBarValues&b){
  return a.hp==b.hp&&a.max_hp==b.max_hp&&a.block==b.block&&a.tracked_block==b.tracked_block&&
   a.poison_damage==b.poison_damage&&a.doom==b.doom&&a.infinite==b.infinite;
 }
 void append(unsigned actor,const HealthBarValues&value,uint32_t id,uint32_t at){
  if(actor>=Actors)return;
  if(same(observed[actor],value)&&observed_identity[actor]==id)return;
  observed[actor]=value;observed_identity[actor]=id;
  if(int32_t(at-end_at)>0)end_at=at;
  if(overflow)return;
  // Hooks at one command boundary may touch the same actor several times.
  // Keep their final displayed value without wasting one entry per hook.
  for(unsigned i=count;i>head&&cues[i-1].at==at;--i)if(cues[i-1].actor==actor){cues[i-1]={value,id,at,uint8_t(actor)};return;}
  if(count==Capacity&&head){for(unsigned i=head;i<count;++i)cues[i-head]=cues[i];count-=head;head=0;}
  if(count==Capacity){overflow=true;return;}
  cues[count++]={value,id,at,uint8_t(actor)};
 }
};
static_assert(sizeof(CombatFeedback)<=8*1024,"Combat display must remain a small, bounded App projection");
}
