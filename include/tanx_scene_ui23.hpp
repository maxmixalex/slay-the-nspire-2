#pragma once
#include <stdint.h>
#include "ancient_scene_ui23.hpp"

namespace spire::tanx_scene_ui23 {
constexpr unsigned AuxiliaryFirst=ancient_scene_ui23::AuxiliaryCount;
constexpr unsigned AuxiliaryCount=AuxiliaryFirst+1;
constexpr unsigned FirstImage=ancient_scene_ui23::Images,Images=FirstImage+7;
constexpr unsigned ParameterBytes=66048;
inline unsigned word(const uint8_t*p){return p[0]|unsigned(p[1])<<8;}
template<class Picture> inline bool parameters(Picture p){
 if(!p.pixels||p.w!=256||p.h!=129||p.format||p.alpha||p.palette)return false;
 const uint16_t expected[]={0x5458,1,FirstImage,FirstImage+1,FirstImage+2,FirstImage+3,
  FirstImage+4,FirstImage+5,200,800,200,2472,3316,200,256,256};
 for(unsigned i=0;i<16;++i)if(word(p.pixels+i*2)!=expected[i])return false;
 return true;
}
// Q8 texel interpolation retains the fractional alpha until the final lookup.
// Every integer alpha pair is source-exact; subtexel fixed-point interpolation
// is separately measured against the floating shader (not called byte-exact).
inline unsigned bilerp(unsigned a,unsigned b,unsigned c,unsigned d,unsigned fx,unsigned fy){
 return (a*(256-fx)*(256-fy)+b*fx*(256-fy)+c*(256-fx)*fy+d*fx*fy+128)>>8;
}
inline unsigned phase(uint32_t clock){return unsigned(uint64_t(clock%10000u)*65536/10000)+32768u;}
struct Smoke {
 const uint8_t*pages[4]{};const uint8_t*gradient=nullptr,*lookup=nullptr;
 unsigned at(int x,int y)const{
  x%=200;y%=800;if(x<0)x+=200;if(y<0)y+=800;
  return pages[unsigned(y)/200][unsigned(y)%200*200+unsigned(x)];
 }
 unsigned smoke_q8(int u,int v)const{
  const int x=(u&65535)*200-32768,y=(v&65535)*800-32768;
  const unsigned fx=unsigned(x&65535),fy=unsigned(y&65535);
  const unsigned top=(at(x>>16,y>>16)*(65536-fx)+at((x>>16)+1,y>>16)*fx+128)>>8;
  const unsigned bottom=(at(x>>16,(y>>16)+1)*(65536-fx)+at((x>>16)+1,(y>>16)+1)*fx+128)>>8;
  return (top*(65536-fy)+bottom*fy+32768)>>16;
 }
 unsigned gradient_q8(int v)const{
  const int y=(v&65535)*200-32768;int a=(y>>16)%200;if(a<0)a+=200;
  const unsigned f=unsigned(y&65535);
  return (gradient[a]*(65536-f)+gradient[(a+1)%200]*f+128)>>8;
 }
 unsigned smooth(unsigned a,unsigned b)const{
  const unsigned ai=a>>8,bi=b>>8,ax=ai<255?ai+1:255,bx=bi<255?bi+1:255;
  return (bilerp(lookup[ai*256+bi],lookup[ax*256+bi],lookup[ai*256+bx],lookup[ax*256+bx],a&255,b&255)+128)>>8;
 }
 unsigned alpha(int u,int v,unsigned scroll)const{return smooth(smoke_q8(u,v+int(scroll)),gradient_q8(v));}
};
}
