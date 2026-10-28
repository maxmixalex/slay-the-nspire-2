#pragma once
#include "core.hpp"
#include "health_bar.hpp"
#include "combat_command_timing.hpp"
#include "enemy_command_program.hpp"

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
 void append(const Pile&p,uint32_t at){append(capture(p),at);}
 void append(const Cards&cards,uint32_t at){
  if(same(observed,cards))return;observed=cards;
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
 struct Commands {
  uint32_t base=0,milliseconds=0;
  uint16_t cursor=0,end=0,operation=0,card_wait=0;
  int16_t state=-1;uint8_t card_pile=255;bool active=false,card_batch=false,first_speech=false;
  uint32_t tick()const{return base+(milliseconds+32u)/33u;}
  void begin(unsigned model,int move,uint32_t at,int visual_state){
   *this={};base=at;state=int16_t(visual_state);
   if(const auto*row=enemy_command_program::find(model,move)){cursor=row->first;end=uint16_t(row->first+row->count);active=true;}
  }
  void align(uint32_t at){if(int32_t(at-tick())>0)milliseconds=(at-base)*33u;}
  uint32_t wait(uint32_t at,unsigned duration){align(at);milliseconds+=duration;return tick();}
 } commands{};
 // AttackCommand awaits every hit, whereas all targets of one hit share
 // its Damage command. Keep that distinction before the value coalescer.
 struct Attack {
  uint32_t base=0,clock_ms=0;uint16_t delay_ms=0,wait_ms=0;int16_t card=-1;
  uint8_t targets=0;bool active=false,hit=false,next=false,once=false;
  void begin(uint32_t at,unsigned delay,unsigned wait=0,bool only_once=false,int instance=-1){
   base=at;clock_ms=0;delay_ms=uint16_t(delay);wait_ms=uint16_t(wait);card=int16_t(instance);
   targets=0;active=true;hit=next=false;once=only_once;
  }
  uint32_t impact(uint32_t at,unsigned target,bool fast){
   const uint8_t bit=uint8_t(1u<<target);
   if(!hit||next||(targets&bit)){
    // A hook may await another command between hits. Preserve that time;
    // otherwise retain sub-tick precision across long multi-hit chains.
    if(int32_t(at-(base+(clock_ms+32)/33))>0)clock_ms=(at-base)*33;
    if(hit)clock_ms+=fast?100:200;
    if(!hit||!once)clock_ms+=delay_ms;
    clock_ms+=wait_ms;targets=0;hit=true;next=false;
   }
   targets|=bit;return base+(clock_ms+32)/33;
  }
  uint32_t finish(bool fast,bool ending=false){active=false;return base+(hit?(clock_ms+(ending?0u:fast?100u:200u)+32)/33:clock_ms/33);}
 } attacks[MaxAutoFrames+1]{},enemy_attack{};
 // Canonical block/tracked-block/doom are signed16 facts; HP, maximum HP
 // and the derived poison series need their full32 bits. Place the actor
 // and infinite flag in the remaining two bytes rather than repeating the
 // padding of HealthBarValues in every queued sample. No packed/unaligned
 // accesses, narrowed HP, smaller queue or change to saved State is needed.
 struct Cue {
  uint32_t identity=0,at=0;int32_t hp=0,max_hp=0,poison_damage=0;
  int16_t block=0,tracked_block=0,doom=0;uint8_t actor=0;bool infinite=false;
  Cue()=default;
  Cue(const HealthBarValues&v,uint32_t id,uint32_t when,uint8_t who):
   identity(id),at(when),hp(v.hp),max_hp(v.max_hp),poison_damage(v.poison_damage),
   block(int16_t(v.block)),tracked_block(int16_t(v.tracked_block)),doom(int16_t(v.doom)),actor(who),infinite(v.infinite){}
  HealthBarValues value()const{return {hp,max_hp,block,tracked_block,poison_damage,doom,infinite};}
 };
 static_assert(sizeof(Cue)==28,"Health playback keeps full HP with aligned canonical-width facts");
 struct DamageCue {uint32_t identity=0,at=0;int32_t amount=0;uint8_t actor=0,flags=0,shake_strength=0;};
 HealthBarValues shown[Actors]{},observed[Actors]{};
 uint32_t identity[Actors]{},observed_identity[Actors]{};
 EnemyIntent intents[MaxEnemies]{};
 uint32_t hide_at[MaxEnemies]{};
 Cue cues[Capacity]{};
 DamageCue damages[Capacity]{};uint16_t damage_head=0,damage_count=0;
 CombatHand hand{};
 uint32_t seed=0,effect_at=0,after_at=0,move_at=0,move_end=0,end_at=0,choice_at=0;
 int floor=-1,combat=-1,encounter=-1,move_slot=-1,move=-1;
 uint16_t head=0,count=0;
 uint8_t hidden=0;
 bool active=false,finished=false,overflow=false,choice_pending=false,damage_overflow=false;
 void reset(){active=finished=overflow=choice_pending=damage_overflow=false;head=count=damage_head=damage_count=0;hidden=0;after_at=0;move_slot=move=-1;commands={};for(auto&a:attacks)a.active=false;enemy_attack.active=false;}
 void append_damage(unsigned actor,const CoreDamageResult&result,uint32_t id,uint32_t at){
  if(actor>=Actors||!(result.amount()>0||(result.flags&(CoreDamageResult::FullyBlocked|CoreDamageResult::ShowZero))))return;
  if(damage_count==Capacity&&damage_head){for(unsigned i=damage_head;i<damage_count;++i)damages[i-damage_head]=damages[i];damage_count-=damage_head;damage_head=0;}
  if(damage_count==Capacity){damage_overflow=true;return;}
  damages[damage_count++]={id,at,result.amount(),uint8_t(actor),result.flags,result.shake_strength};
  if(int32_t(at-end_at)>0)end_at=at;
 }
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
