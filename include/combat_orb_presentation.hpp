#pragma once
#include "combat_card_motion.hpp"

namespace spire {
struct OrbSourcePoint {int32_t x=0,y=0;}; // Q10 original scene pixels.
struct CombatOrbPresentation {
 struct Slot {OrbSourcePoint from{};uint32_t born=0;uint8_t kind=255;};
 Slot items[MaxOrbs]{};
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
  const auto to=target(slots,index);const unsigned age=now-started;if(age>=450)return to;
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
};
static_assert(sizeof(CombatOrbPresentation)<=176,"Orb layout needs no framebuffer or State copy");
}
