#pragma once
#include "status.hpp"

namespace spire {
// Only values used by the power strip and its details are retained. This is
// presentation data: no State copy, save serialization, or gameplay callbacks.
struct CombatStatus {
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
