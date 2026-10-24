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
  Automation=CapturedCard+MaxEnemies,AutomationCount=Automation+MaxPowerInstances,
  Facts=AutomationCount+MaxPowerInstances
 };
 static constexpr unsigned Actors=MaxEnemies+1,FactBase=Actors*StatusKinds,
  Cells=FactBase+Facts,Capacity=256;
 struct Value {
  int32_t amount=0;int16_t auxiliary=0;uint8_t source=0,visible=0;
  bool operator==(const Value&v)const{return amount==v.amount&&auxiliary==v.auxiliary&&source==v.source&&visible==v.visible;}
 };
 struct Cue {Value value{};uint32_t at=0;uint16_t cell=0;};
 Value shown[Cells]{},observed[Cells]{};
 PowerOrderState shown_order{},observed_order{};
 Cue cues[Capacity]{};uint16_t head=0,count=0;bool overflow=false;
 bool powers_frozen=false;
 // PowerAppliedVfx outlives its command; finishing a playback trace must
 // not remove the text. Fixed-size identifiers avoid per-frame strings or
 // a second State snapshot, and stay independent of saved gameplay data.
 static constexpr unsigned PopupCapacity=96;
 struct Popup {uint32_t at=0,identity=0;uint16_t id=0;uint8_t actor=0,buff=0,lane=0,removed=0;};
 static constexpr unsigned DamageCapacity=48;
 struct Damage {uint32_t at=0,identity=0;int32_t amount=0;int16_t vx=0,vy=0;uint8_t actor=0,variation=0,flags=0,shake_strength=0;uint16_t angle=0;};
 Damage damages[DamageCapacity]{};uint8_t damage_count=0;uint32_t cosmetic=0x48a56b39u,pruned_at=UINT32_MAX;
 Popup popups[PopupCapacity]{};uint8_t popup_count=0;
 static constexpr unsigned FlashCapacity=48;
 struct Flash {uint32_t at=0,identity=0;uint16_t id=0;uint8_t actor=0;};
 Flash flashes[FlashCapacity]{};uint8_t flash_count=0;
 uint32_t reward_at=0;bool reward_scheduled=false,had_live_scene=false;
 uint32_t visual_seed=0,star_gain_at=0,star_rotation_mdeg=0,star_tick=0;int visual_floor=-1,visual_combat=-1,visual_encounter=-1;
 StarCounterMotion star_count{};int star_observed=0;bool stars_visible=false,star_zero=true;
 void reset_visuals(){reward_at=0;reward_scheduled=had_live_scene=powers_frozen=false;popup_count=damage_count=flash_count=0;cosmetic=0x48a56b39u;pruned_at=UINT32_MAX;visual_floor=visual_combat=visual_encounter=-1;stars_visible=false;star_observed=0;star_gain_at=star_rotation_mdeg=star_tick=0;star_count={};star_zero=true;}
 static uint32_t popup_ticks(const Popup&p){return p.removed?54u:38u;}
 void expire(uint32_t now){
  if(now==pruned_at)return;pruned_at=now;
  unsigned out=0;for(unsigned i=0;i<popup_count;++i)if(int32_t(now-popups[i].at)<int32_t(popup_ticks(popups[i])))popups[out++]=popups[i];popup_count=uint8_t(out);
  out=0;for(unsigned i=0;i<damage_count;++i)if(int32_t(now-damages[i].at)*33<2000)damages[out++]=damages[i];damage_count=uint8_t(out);
  out=0;for(unsigned i=0;i<flash_count;++i)if(int32_t(now-flashes[i].at)*33<1050)flashes[out++]=flashes[i];flash_count=uint8_t(out);
 }
 void flash(unsigned actor,unsigned id,uint32_t identity,uint32_t at,uint32_t now){
  expire(now);if(id>=StatusKinds||!(PowerFlags[id]&1))return;
  // Multiple modifiers of the same impact share one power flash; distinct
  // commands retain their own lifetime, independently of amount changes.
  for(unsigned i=0;i<flash_count;++i)if(flashes[i].actor==actor&&flashes[i].id==id&&flashes[i].identity==identity&&flashes[i].at==at)return;
  if(flash_count==FlashCapacity){for(unsigned i=1;i<flash_count;++i)flashes[i-1]=flashes[i];--flash_count;}
  flashes[flash_count++]={at,identity,uint16_t(id),uint8_t(actor)};
 }
 void popup(unsigned actor,unsigned id,bool buff,uint32_t identity,uint32_t at,uint32_t now,bool removed=false){
  expire(now);
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
 void damage(unsigned actor,int amount,uint32_t identity,uint32_t at,uint32_t now,uint8_t flags=0,uint8_t strength=0){
  expire(now);
  if(damage_count==DamageCapacity){for(unsigned i=1;i<damage_count;++i)damages[i-1]=damages[i];--damage_count;}
  // NDamageNumVfx's chaotic stream never consumes saved combat RNG.
  damages[damage_count++]={at,identity,amount,int16_t(int(random()%201u)-100),int16_t(-800+int(random()%101u)),uint8_t(actor),uint8_t(random()%101u),flags,strength,uint16_t(random()&4095u)};
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
 static void update_token(PowerOrderState&order,uint16_t token,bool present){
  unsigned at=0;while(at<order.count&&order.entries[at]!=token)++at;
  if(present){if(at==order.count&&order.count<MaxPowerOrderEntries)order.entries[order.count++]=token;}
  else if(at<order.count){for(unsigned i=at+1;i<order.count;++i)order.entries[i-1]=order.entries[i];order.entries[--order.count]=0;}
 }
 void queue_order(uint16_t token,bool present,uint32_t at){
  if(overflow)return;
  if(count==Capacity&&head){for(unsigned i=head;i<count;++i)cues[i-head]=cues[i];count-=head;head=0;}
  if(count==Capacity){overflow=true;return;}
  // Unlike value cues, a remove/reapply pair must not coalesce: its position
  // is the change. These tokens occupy a disjoint compact cell-ID range.
  cues[count++]={Value{present?1:0,0,0,0},at,uint16_t(Cells+token)};
 }
 void capture_order(const PowerOrderState&next,uint32_t at,bool initial){
  if(initial){shown_order=observed_order=next;return;}
  // Power nodes only disappear or append. Keep the longest prefix of the
  // new list that is a subsequence of the old list, so deleting an early
  // icon emits one cue instead of rebuilding every later icon. Both scans
  // are linear and preserve remove/reapply ordering without an extra array.
  unsigned retained=0,scan=0;
  while(retained<next.count){
   while(scan<observed_order.count&&observed_order.entries[scan]!=next.entries[retained])++scan;
   if(scan==observed_order.count)break;++scan;++retained;
  }
  unsigned keep=0;
  for(unsigned i=0;i<observed_order.count;++i){
   if(keep<retained&&observed_order.entries[i]==next.entries[keep])++keep;
   else queue_order(observed_order.entries[i],false,at);
  }
  for(unsigned i=retained;i<next.count;++i)queue_order(next.entries[i],true,at);
  observed_order=next;
 }
 void sync(uint32_t now){
  while(head<count&&int32_t(now-cues[head].at)>=0){const auto&cue=cues[head++];
   if(cue.cell>=Cells)update_token(shown_order,uint16_t(cue.cell-Cells),cue.value.amount!=0);
   else shown[cue.cell]=cue.value;
  }
  if(head==count)head=count=0;
 }
 void finish(){for(unsigned i=0;i<Cells;++i)shown[i]=observed[i];shown_order=observed_order;reset();}
};
static_assert(CombatStatus::Cells+(MaxEnemies<<12)+(15<<8)+255<65536&&EnemyKinds<255,"Status cue indices must fit their compact fields");
static_assert(sizeof(CombatStatus)<=64*1024,"Power playback must have a fixed, bounded memory cost");
}
