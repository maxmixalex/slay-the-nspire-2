#pragma once
#include <stdint.h>

namespace spire::phobia_swarm {
// Original PhobiaModeVisuals/SwarmParticles:100 continuous particles, a2s
// lifetime and2s prewarm. Reconstruct each particle from its birth number;
// repainting/pausing never advances gameplay RNG or allocates a particle pool.
constexpr unsigned Count=100,LifetimeMs=2000,SpacingMs=LifetimeMs/Count;
struct Point {int x=0,y=0,scale=0;unsigned age=0;}; // local source pixels in Q8; scale in Q10.
#include "phobia_swarm_curves.inc"
inline unsigned random(uint32_t&seed){seed=seed*1664525u+1013904223u;return seed;}
inline unsigned root(unsigned value){unsigned result=0,bit=1u<<16;
 while(bit){if(value>=result+bit){value-=result+bit;result=(result>>1)+bit;}else result>>=1;bit>>=2;}return result;}
inline int sine(int angle){
 static constexpr int16_t quarter[]={0,100,200,297,392,483,569,650,724,792,851,903,946,980,1004,1019,1024};
 unsigned phase=unsigned(angle)&4095u;const int sign=phase>=2048?-1:1;phase%=2048;if(phase>1024)phase=2048-phase;
 const unsigned i=phase/64,p=phase%64;return sign*(i==16?1024:(quarter[i]*int(64-p)+quarter[i+1]*int(p))/64);
}
inline int curve(const int16_t*values,unsigned phase){
 const unsigned i=phase>>9,p=phase&511u;return values[i]+(int(values[i+1])-values[i])*int(p)/512;
}
inline Point sample(uint32_t seed,unsigned slot,uint32_t milliseconds){
 if(slot>=Count)return {};
 const unsigned phase_ms=milliseconds%LifetimeMs,offset=slot*SpacingMs;
 const unsigned age=(phase_ms+LifetimeMs-offset)%LifetimeMs;
 const uint32_t birth=milliseconds/LifetimeMs+(phase_ms>=offset);
 seed^=(slot+1)*0x9e3779b9u;seed^=birth*0x85ebca6bu;random(seed);
 // Godot4.5.1 samples ring area uniformly, by interpolating squared radii.
 const unsigned radius=root(2500u+(random(seed)>>22)*157500u/1024u);
 const int angle=int(random(seed)>>20),velocity_angle=int(random(seed)>>20);
 const unsigned speed=100u+(random(seed)>>24)*100u/255u;
 const int orbit=int(random(seed)>>21)-1024,directional=int(random(seed)>>21)-1024;
 const unsigned base_scale=1024u+(random(seed)>>23);
 const unsigned phase=age*65536u/LifetimeMs;
 const int turn=curve(Orbit,phase)*orbit/1024;
 Point out{int(radius)*sine(angle-turn+1024)/4,int(radius)*sine(angle-turn)/4,
           int(base_scale)*curve(Scale,phase)/1024,age};
 // Integrate velocity in the rotating frame at eight bounded midpoints.
 // The authored angular integral preserves its quick orbit reversals; the
 // LCD quadrature costs no history buffer or per-frame floating point.
 const int travel=int(speed*age*256u/8000u);
 for(unsigned step=0;step<8;++step){
  const unsigned mid=phase*(step*2+1)/16;
  const int rotation=curve(Orbit,mid)*orbit/1024-turn;
  out.x+=travel*sine(velocity_angle+rotation+1024)/1024;
  out.y+=travel*sine(velocity_angle+rotation)/1024;
  const int drift=(curve(Directional,mid)*directional/1024)*int(age)/32000;
  out.x+=drift*sine(rotation+1024)/1024;out.y+=drift*sine(rotation)/1024;
 }
 return out;
}
}
