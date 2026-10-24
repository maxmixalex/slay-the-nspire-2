#pragma once
#include "status.hpp"
#include "star_counter_motion.hpp"

namespace spire {
// Only values used by the power strip and its details are retained. This is
// presentation data: no State copy, save serialization, or gameplay callbacks.
struct CombatStatus {
 static constexpr uint8_t PowerFlags[]={
#include "combat_power_visuals.inc"
 };
 static_assert(sizeof PowerFlags==StatusKinds,"Every power needs original metadata");
 enum Fact {
  CrimsonDamage,InfernoDamage,StarsGained,OstyMax,OstyAttacks,PhysicalStarted,
  FeralUsed,RegentCards,PaleActivated,VoidPlayed,EtherealPlayed,Drawn,OrbSlots,OrbCount,
  Panache,PanacheCards=Panache+MaxPowerInstances,
  Boulder=PanacheCards+MaxPowerInstances,BombTurns=Boulder+MaxPowerInstances,
  BombDamage=BombTurns+MaxBombs,Monologue=BombDamage+MaxBombs,
  MonologueApplied=Monologue+MaxPowerInstances,NightmareCard=MonologueApplied+MaxPowerInstances,
  NightmareActive=NightmareCard+MaxNightmares,Orbit=NightmareActive+MaxNightmares,
  OrbitRemainder=Orbit+MaxPowerInstances,OrbKind=OrbitRemainder+MaxPowerInstances,
  OrbPassive=OrbKind+MaxOrbs,OrbEvoke=OrbPassive+MaxOrbs,CapturedCard=OrbEvoke+MaxOrbs,
  Facts=CapturedCard+MaxEnemies
 };
 static constexpr unsigned Actors=MaxEnemies+1,FactBase=Actors*StatusKinds,
  Cells=FactBase+Facts,Capacity=256;
 struct Value {
  int32_t amount=0;int16_t auxiliary=0;uint8_t source=0,visible=0;
  bool operator==(const Value&v)const{return amount==v.amount&&auxiliary==v.auxiliary&&source==v.source&&visible==v.visible;}
 };
 struct Cue {Value value{};uint32_t at=0;uint16_t cell=0;};
 Value shown[Cells]{},observed[Cells]{};
 Cue cues[Capacity]{};uint16_t head=0,count=0;bool overflow=false;
 // PowerAppliedVfx outlives its command; finishing a playback trace must
 // not remove the text. Fixed-size identifiers avoid per-frame strings or
 // a second State snapshot, and stay independent of saved gameplay data.
 static constexpr unsigned PopupCapacity=96;
 struct Popup {uint32_t at=0,identity=0;uint16_t id=0;uint8_t actor=0,buff=0,lane=0,removed=0;};
 static constexpr unsigned DamageCapacity=48;
 struct Damage {uint32_t at=0,identity=0;int32_t amount=0;int16_t vx=0,vy=0;uint8_t actor=0,variation=0;};
 Damage damages[DamageCapacity]{};uint8_t damage_count=0;uint32_t cosmetic=0x48a56b39u;
 Popup popups[PopupCapacity]{};uint8_t popup_count=0;
 uint32_t reward_at=0;bool reward_scheduled=false;
 uint32_t visual_seed=0,star_gain_at=0,star_rotation_mdeg=0,star_tick=0;int visual_floor=-1,visual_combat=-1,visual_encounter=-1;
 StarCounterMotion star_count{};int star_observed=0;bool stars_visible=false,star_zero=true;
 void reset_visuals(){reward_at=0;reward_scheduled=false;popup_count=damage_count=0;cosmetic=0x48a56b39u;visual_floor=visual_combat=visual_encounter=-1;stars_visible=false;star_observed=0;star_gain_at=star_rotation_mdeg=star_tick=0;star_count={};star_zero=true;}
 static uint32_t popup_ticks(const Popup&p){return p.removed?54u:38u;}
 void popup(unsigned actor,unsigned id,bool buff,uint32_t identity,uint32_t at,uint32_t now,bool removed=false){
  unsigned out=0;for(unsigned i=0;i<popup_count;++i)if(int32_t(now-popups[i].at)<int32_t(popup_ticks(popups[i])))popups[out++]=popups[i];popup_count=uint8_t(out);
  for(unsigned i=0;i<popup_count;++i)if(popups[i].actor==actor&&popups[i].id==id&&popups[i].at==at&&bool(popups[i].removed)==removed){popups[i].buff=uint8_t(buff);return;}
  // Three readable rows keep simultaneous powers distinct at calculator
  // scale. A fourth entry waits for a row, without delaying core commands.
  uint32_t available[3]={at,at,at};
  for(unsigned i=0;i<popup_count;++i){const auto&p=popups[i];if(p.actor!=actor||p.identity!=identity)continue;
   const uint32_t end=p.at+popup_ticks(p);if(int32_t(end-available[p.lane])>0)available[p.lane]=end;}
  unsigned lane=0;for(unsigned i=1;i<3;++i)if(int32_t(available[i]-available[lane])<0)lane=i;
  if(int32_t(available[lane]-at)>0)at=available[lane];
  if(popup_count==PopupCapacity){for(unsigned i=1;i<popup_count;++i)popups[i-1]=popups[i];--popup_count;}
  popups[popup_count++]={at,identity,uint16_t(id),uint8_t(actor),uint8_t(buff),uint8_t(lane),uint8_t(removed)};
 }
 unsigned random(){cosmetic^=cosmetic<<13;cosmetic^=cosmetic>>17;cosmetic^=cosmetic<<5;return cosmetic;}
 void damage(unsigned actor,int amount,uint32_t identity,uint32_t at,uint32_t now){
  unsigned out=0;for(unsigned i=0;i<damage_count;++i)if(int32_t(now-damages[i].at)*33<2000)damages[out++]=damages[i];damage_count=uint8_t(out);
  if(damage_count==DamageCapacity){for(unsigned i=1;i<damage_count;++i)damages[i-1]=damages[i];--damage_count;}
  // NDamageNumVfx's chaotic stream never consumes saved combat RNG.
  damages[damage_count++]={at,identity,amount,int16_t(int(random()%201u)-100),int16_t(-800+int(random()%101u)),uint8_t(actor),uint8_t(random()%101u)};
 }
 void reset(){head=count=0;overflow=false;}
 void append(unsigned cell,Value value,uint32_t at){
  if(cell>=Cells||observed[cell]==value)return;observed[cell]=value;
  if(overflow)return;
  for(unsigned i=count;i>head&&cues[i-1].at==at;--i)if(cues[i-1].cell==cell){cues[i-1].value=value;return;}
  if(count==Capacity&&head){for(unsigned i=head;i<count;++i)cues[i-head]=cues[i];count-=head;head=0;}
  if(count==Capacity){overflow=true;return;}
  cues[count++]={value,at,uint16_t(cell)};
 }
 void sync(uint32_t now){
  while(head<count&&int32_t(now-cues[head].at)>=0){const auto&cue=cues[head++];shown[cue.cell]=cue.value;}
  if(head==count)head=count=0;
 }
 void finish(){for(unsigned i=0;i<Cells;++i)shown[i]=observed[i];reset();}
};
static_assert(CombatStatus::Cells<65536&&EnemyKinds<255,"Status cue indices must fit their compact fields");
static_assert(sizeof(CombatStatus)<=64*1024,"Power playback must have a fixed, bounded memory cost");
}
