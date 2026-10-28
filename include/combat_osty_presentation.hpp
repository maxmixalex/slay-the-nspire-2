#pragma once
#include "combat_card_motion.hpp"

namespace spire {
// NCreature.OstyScaleToSize changes Visuals over750ms and refreshes the
// hitbox only at completion. Creature position has the ordinary linear
// PropertyTweener; the Visuals scale explicitly selects Sine/InOut.
// Source coordinates retain formation precision and never enter SaveData.
struct CombatOstyPresentation {
 uint32_t started=0;
 int32_t from_x=0,to_x=0;
 int16_t maximum=-1;
 uint16_t from_scale=1024,to_scale=1024,bounds_scale=1024;
 bool alive=false;
 static constexpr unsigned Duration=750;
 static uint16_t size(int hp){if(hp<0)hp=0;if(hp>150)hp=150;return uint16_t(1024+1024*hp/150);}
 unsigned elapsed(uint32_t now)const{return int32_t(now-started)<0?0:now-started;}
 int x(uint32_t now)const{const unsigned t=elapsed(now);return t>=Duration?to_x:from_x+int(int64_t(to_x-from_x)*t/Duration);}
 int scale(uint32_t now)const{
  const unsigned t=elapsed(now);if(t>=Duration)return to_scale;
  const int ease=(1024-CombatCardMotion::sine(int(t*2048u/Duration)+1024))/2;
  return from_scale+(int(to_scale)-from_scale)*ease/1024;
 }
 void bind(uint32_t now,int hp,bool living,int position){
  started=now;from_x=to_x=position;maximum=int16_t(hp);alive=living;
  from_scale=to_scale=bounds_scale=size(living?hp:0);
 }
 void observe(uint32_t now,int hp,bool living,int position,uint32_t cue){
  if(elapsed(now)>=Duration)bounds_scale=to_scale;
  if(maximum==hp&&alive==living)return;
  const int current_x=x(now),current_scale=scale(now);
  maximum=int16_t(hp);alive=living;from_x=current_x;to_x=position;
  from_scale=uint16_t(current_scale);to_scale=size(living?hp:0);started=cue;
 }
};
static_assert(sizeof(CombatOstyPresentation)<=28,"Osty growth retains numeric presentation only");
}
