#pragma once
#include "core.hpp"
#include "health_bar.hpp"

namespace spire {
// NDoomVfx owns a detached body, not the creature's ordinary death track.
// Keep its immutable registration and clip sample; no viewport allocation or
// gameplay snapshot is needed on the LCD renderer.
struct CombatDoomPresentation {
 enum:uint8_t {Active=1,Disappear=2,Frozen=4,Loop=8,Shared=16,OverlayApplied=32};
 struct Body {
  uint32_t at=0,identity=0;
  int16_t clip=-1,x=0,y=0,ground_x=0,ground_y=0,width=0,height=0,scale=1024;
  uint16_t sample_ms=0,unit=780,width_q12=0;
  uint8_t flags=0,actor=0;
  bool started(uint32_t now)const{return (flags&Active)&&int32_t(now-at)>=0;}
  bool alive(uint32_t now)const{return started(now)&&now-at<3500u;}
  unsigned sink(uint32_t now)const{
   if(!started(now)||!(flags&Disappear)||now-at<=750u)return 0;
   const unsigned age=now-at-750u;if(age>=750u)return unsigned(height)*3u/2u;
   const unsigned q=4096u-health_bar::expo_out(1000u-age*1000u/750u);
   return unsigned(height)*3u*q/8192u;
  }
  unsigned width_scale(uint32_t now)const{
   if(!started(now))return 0;const unsigned age=now-at;
   if(age<500u){const unsigned t=(500u-age)*4096u/500u;return unsigned(width_q12)*(4096u-t*t/4096u*t/4096u)/4096u;}
   // ShowOrHide(0,.25) substitutes a300ms delay and a width*.15s
   // Cubic.In contraction, rather than immediately stopping the emitter.
   if(age<1800u)return width_q12;
   const unsigned duration=unsigned(width_q12)*150u/4096u;
   if(!duration||age-1800u>=duration)return 0;
   const unsigned t=(age-1800u)*4096u/duration;
   return unsigned(width_q12)*(4096u-t*t/4096u*t/4096u)/4096u;
  }
 } bodies[MaxEnemies+1]{};
 uint32_t overlay_at=0;
 uint16_t overlay_from=0;
 bool overlay_active=false;
 unsigned overlay(uint32_t now)const{
  if(!overlay_active||int32_t(now-overlay_at)<0)return 0;const unsigned age=now-overlay_at;
  if(age<1000u)return overlay_from+(4096u-overlay_from)*health_bar::expo_out(age)/4096u;
  if(age<2200u)return 4096u;
  return age<2700u?(2700u-age)*4096u/500u:0u;
 }
 void restart_overlay(uint32_t at){overlay_from=uint16_t(overlay(at));overlay_at=at;overlay_active=true;}
 void sync_overlay(uint32_t now){
  // Multiple Doom commands can settle synchronously while their visual
  // cues are in the future. Restart only when each actual cue is reached.
  for(unsigned pass=0;pass<=MaxEnemies;++pass){Body*next=nullptr;
   for(auto&b:bodies)if(b.started(now)&&!(b.flags&OverlayApplied)&&(!next||int32_t(b.at-next->at)<0))next=&b;
   if(!next)break;restart_overlay(next->at);next->flags|=OverlayApplied;
  }
 }
 void reset(){*this=CombatDoomPresentation{};}
};
static_assert(sizeof(CombatDoomPresentation)<=384,"Doom stores bounded clip facts, never a viewport");
}
