#pragma once
#include "screen_shake.hpp"
namespace spire {
// NSleepingVfx's own lifecycle, separate from sleep_loop on the skeleton.
// Stop stops emission; the four-second particles already born finish.
struct CreatureSleepPresentation {
 uint32_t start=0,stopped=0;bool active=false,started=false,finished=false;
 void stop(uint32_t now){if(finished)return;active=false;stopped=now;finished=true;}
 void observe(bool asleep,uint32_t now){
  if(asleep&&!active&&!finished){start=now;stopped=0;active=started=true;}
  else if(!asleep&&active){active=false;stopped=now;}
 }
 bool alive(uint32_t now)const{return started&&(active||uint32_t(now-stopped)<4000u);}
 struct Z {int x=0,y=0,size=0;unsigned alpha=0;bool visible=false;};
 Z sample(unsigned index,uint32_t now,uint32_t seed)const{
  Z out;if(index>=4||!alive(now))return out;
  const uint32_t elapsed=now-start;uint32_t born=(elapsed/4000u)*4000u+index*1000u;
  if(born>elapsed){if(born<4000)return out;born-=4000;}
  if(!active&&born>uint32_t(stopped-start))return out;
  const uint32_t hash=ScreenShakePresentation::hash(seed^index*0x9e3779b9u^born),life=3600u+hash%401u,age=elapsed-born;
  if(age>=life)return out;
  // Source direction(.5,-1), speed120..140, friction .04 and20px origin
  // jitter. Bounded rational integration maps the drag to integer source
  // units; dense shader turbulence is not a gameplay random draw.
  const unsigned speed=120u+(hash>>9)%21u;
  const int distance=int(uint64_t(speed)*age*1000u/(1000000u+speed*age));
  out.x=20+int((hash>>17)%41u)-20+distance*458/1024;
  out.y=-20+int((hash>>23)%41u)-20-distance*916/1024;
  const unsigned q=age*1024u/life,grow=q>=512?1024u:512u+q;
  out.size=int((55u+(hash>>13)%11u)*grow/1024u);
  out.alpha=q<512?255u:(1024u-q)*255u/512u;out.visible=out.alpha>0;return out;
 }
};
static_assert(sizeof(CreatureSleepPresentation)<=12,"Sleeping effects keep clocks, not particle buffers");
}
