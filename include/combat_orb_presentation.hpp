#pragma once
#include "combat_card_motion.hpp"

namespace spire {
struct OrbSourcePoint {int32_t x=0,y=0;}; // Q10 original scene pixels.
struct CombatOrbPresentation {
 static constexpr unsigned Capacity=128;
 struct Slot {OrbSourcePoint from{};uint32_t born=0;uint8_t kind=255;};
 struct Ghost {OrbSourcePoint point{};uint32_t at=0,born=0;int16_t passive=0,evoke=0;uint8_t kind=255,slot=0;};
 struct Cue {uint32_t at=0;int16_t passive=0,evoke=0;uint8_t event=0,slot=0,slots=0,kind=255;};
 Slot items[MaxOrbs]{};Ghost ghosts[MaxOrbs]{};Cue cues[Capacity]{};
 uint16_t head=0,count=0;uint8_t ghost_next=0;bool overflow=false;
 uint32_t started=0;
 uint8_t slots=0;
 bool bound=false;
 static OrbSourcePoint target(unsigned count,unsigned index){
  if(!count||index>=count)return {};
  // NOrbManager uses the same -150..-25 degree arc even for a single
  // slot, whose first angle is finite before the unused infinite step.
  const int phase=count==1?-150*4096/360:int((-150*int(count-1)+125*int(index))*4096/(360*int(count-1)));
  const int radius=(225*7+75*(int(count)-3))*1024/7;
  return {-radius*CombatCardMotion::sine(phase+1024)/1024,radius*CombatCardMotion::sine(phase)/1024};
 }
 OrbSourcePoint position(unsigned index,uint32_t now)const{
  if(index>=slots)return {};
  const auto to=target(slots,index);const unsigned age=int32_t(now-started)<0?0u:now-started;if(age>=450)return to;
  const int ease=(1024-CombatCardMotion::sine(int(age)*2048/450+1024))/2;const auto from=items[index].from;
  return {from.x+int(int64_t(to.x-from.x)*ease/1024),from.y+int(int64_t(to.y-from.y)*ease/1024)};
 }
 void layout(unsigned count,uint32_t now){
  if(count>MaxOrbs)count=MaxOrbs;
  if(bound&&slots==count)return;
  for(unsigned i=0;i<count;++i){
   if(bound&&i<slots)items[i].from=position(i,now);
   else{items[i]={};items[i].born=now;}
  }
  slots=uint8_t(count);started=now;bound=true;
 }
 void model(unsigned index,unsigned kind,uint32_t now){
  if(index>=slots)return;auto&slot=items[index];
  if(slot.kind!=kind){slot.kind=uint8_t(kind);slot.born=now;}
 }
 void restart(uint32_t now){
  for(unsigned i=0;i<slots;++i)items[i].from=position(i,now);
  started=now;
 }
 void apply(const Cue&cue){
  const uint32_t now=cue.at;
  if(cue.event==CoreOrbNotice::Capacity){layout(cue.slots,now);return;}
  if(cue.slot>=slots)return;
  if(cue.event==CoreOrbNotice::Removed){
   restart(now);const auto old=items[cue.slot];auto&ghost=ghosts[ghost_next];ghost_next=uint8_t((ghost_next+1)%MaxOrbs);
   ghost={old.from,now,old.born,cue.passive,cue.evoke,old.kind,cue.slot};
   for(unsigned i=cue.slot+1;i<slots;++i)items[i-1]=items[i];
   items[slots-1]={};items[slots-1].born=now;
  }else if(cue.event==CoreOrbNotice::Channeled){
   // AddOrb replaces the empty holder at its CURRENT position, then
   // restarts the same 450 ms layout even when capacity is unchanged.
   restart(now);items[cue.slot].kind=cue.kind;items[cue.slot].born=now;
  }
 }
 void advance(uint32_t now){
  while(head<count&&int32_t(now-cues[head].at)>=0)apply(cues[head++]);
  if(head==count)head=count=0;
 }
 void enqueue(Cue cue,uint32_t now){
  advance(now);
  if(!count&&int32_t(now-cue.at)>=0){apply(cue);return;}
  if(count==Capacity&&head){for(unsigned i=head;i<count;++i)cues[i-head]=cues[i];count-=head;head=0;}
  if(count==Capacity){overflow=true;return;}
  unsigned insert=count;while(insert>head&&int32_t(cue.at-cues[insert-1].at)<0){cues[insert]=cues[insert-1];--insert;}
  cues[insert]=cue;++count;
 }
 OrbSourcePoint activation(unsigned slot,unsigned kind,bool evoke,uint32_t at)const{
  // Predict only bounded presentation facts, never a canonical State or
  // RNG. This supplies the true source point during an unfinished tween.
  CombatOrbPresentation predicted=*this;predicted.advance(at);
  if(evoke){const auto&last=predicted.ghosts[(predicted.ghost_next+MaxOrbs-1)%MaxOrbs];
   if(last.kind==kind&&last.slot==slot&&last.at==at)return last.point;
  }
  return predicted.position(slot,at);
 }

};
static_assert(sizeof(CombatOrbPresentation)<=2048,"Orb timeline needs no framebuffer or State copy");
}
