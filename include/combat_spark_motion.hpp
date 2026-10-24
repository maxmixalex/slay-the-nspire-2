#pragma once
#include <stdint.h>

namespace spire {
// NHitSparkVfx/NBlockSparkVfx restart only the two common speck systems,
// glow and streak/ring. The old StandardSparks/SmallSparks nodes are hidden.
// This bounded display state follows the visible materials at60Hz. It does
// not consume combat RNG, retain creatures, or keep another framebuffer.
struct CombatSparkMotion {
 static constexpr unsigned Capacity=48,Particles=10;
 struct Particle {
  int32_t distance=0; // Q10 calculator pixels, along the emission direction.
  uint16_t speed=0,radial=0,damping=0,scale=0,lifetime=0,angle=0;
  uint8_t variation=0;
 };
 struct Burst {
  Particle particles[Particles]{};
  uint32_t at=0,seed=0;
  int16_t x=0,y=0;
  uint16_t stepped=0;
  uint8_t steps=0,blocked=0;
 };
 Burst bursts[Capacity]{};
 uint8_t count=0;uint32_t clock=UINT32_MAX;
 static uint32_t random(uint32_t&v){v^=v<<13;v^=v>>17;v^=v<<5;return v;}
 static int bounded(int value,int low=0,int high=1024){return value<low?low:value>high?high:value;}
 // Cubic Hermite curve with authored endpoint tangents, inQ10. Tangents
 // are derivatives in normalized source curve space, not tween easing.
 static int curve(int t,int y0,int y1,int m0=0,int m1=0){
  t=bounded(t);const int t2=t*t/1024,t3=t2*t/1024;
  return ((2*t3-3*t2+1024)*y0+(t3-2*t2+t)*m0+(-2*t3+3*t2)*y1+(t3-t2)*m1)/1024;
 }
 static int segment(int t,int x0,int x1,int y0,int y1,int m0,int m1){
  return curve((t-x0)*1024/(x1-x0),y0,y1,m0*(x1-x0)/1024,m1*(x1-x0)/1024);
 }
 static unsigned progress(unsigned age,unsigned life){return age>=life?1024u:age*1024u/life;}
 static unsigned velocity(const Particle&p,unsigned age){
  return p.speed+unsigned(bounded(curve(int(progress(age,p.lifetime)),1024,0,0,-2956)))*p.radial/1024u;
 }
 void reset(){count=0;clock=UINT32_MAX;}
 void emit(uint32_t at,int x,int y,bool blocked,uint32_t seed){
  if(count==Capacity){for(unsigned i=1;i<count;++i)bursts[i-1]=bursts[i];--count;}
  auto&b=bursts[count++];b={};b.at=at;b.x=int16_t(x);b.y=int16_t(y);b.blocked=uint8_t(blocked);b.seed=seed?seed:1u;
  uint32_t rng=b.seed;
  for(unsigned i=0;i<Particles;++i){auto&p=b.particles[i];const bool bright=i<6;
   p.speed=uint16_t((bright?1500:400)+random(rng)%(bright?1001u:201u));
   p.radial=uint16_t(150+random(rng)%101u);
   p.damping=uint16_t((bright?307:512)+random(rng)%257u);
   p.scale=uint16_t(307+random(rng)%(bright?155u:104u));
   p.lifetime=uint16_t((bright?375:200)+random(rng)%(bright?376u:801u));
   p.angle=uint16_t(random(rng)&4095u);p.variation=uint8_t(random(rng)%256u);
  }
  clock=UINT32_MAX;
 }
 void step(uint32_t now){
  if(clock==now)return;clock=now;
  unsigned out=0;
  for(unsigned n=0;n<count;++n){auto&b=bursts[n];const int age=int32_t(now-b.at);
   if(age>=1000)continue;
   if(age>=0)while(b.steps<60){const unsigned next=(unsigned(b.steps+1)*1000u+59u)/60u;if(next>unsigned(age))break;
    const unsigned dt=next-b.stepped;
    for(auto&p:b.particles){if(next>=p.lifetime)continue;
     const unsigned life=progress(next,p.lifetime),damp=unsigned(curve(int(life*1024u/361u),0,1024))*p.damping/1024u;
     // Godot's damping-as-friction uses speed²*damping*0.05*dt.
     // Divide before multiplying: the authored maximum2500 keeps all
     // intermediates within32bits on the calculator.
     const unsigned loss=(unsigned(p.speed)*p.speed/1024u)*damp*dt/20000u;
     p.speed=uint16_t(loss>=p.speed?0:p.speed-loss);
     p.distance+=int32_t(velocity(p,next)*dt*1024u/6000u);
    }
    b.stepped=uint16_t(next);++b.steps;
   }
   if(out!=n)bursts[out]=b;++out;
  }count=uint8_t(out);
 }
 static int radius(const Burst&b,const Particle&p,unsigned age){
  return p.distance+int(velocity(p,age)*(age-b.stepped)*1024u/6000u);
 }
};
static_assert(sizeof(CombatSparkMotion)<12*1024,"Hit particles use a bounded display-only pool");
}
