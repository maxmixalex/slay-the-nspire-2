#pragma once
#include <stdint.h>
#include "fake_merchant_presentation.hpp"

namespace spire::nonu_fog_ui23 {
constexpr unsigned Auxiliary=7222,FirstImage=59712,Images=59717;
inline unsigned word(const uint8_t*p){return p[0]|unsigned(p[1])<<8;}
inline int32_t signed_word(const uint8_t*p){
 const uint32_t n=word(p)|uint32_t(word(p+2))<<16;
 return n<=0x7fffffffu?int32_t(n):-1-int32_t(0xffffffffu-n);
}
struct Parameters {
 const uint8_t*data=nullptr;
 template<class Picture>bool read(Picture p){
  if(!p.pixels||p.w!=256||p.h!=4||p.format||p.alpha||p.palette)return false;
  const auto*b=p.pixels;
  if(b[0]!='N'||b[1]!='F'||b[2]!='G'||b[3]!='1'||word(b+4)!=1||word(b+6)!=4||
     word(b+8)!=FirstImage||word(b+10)!=200||word(b+12)!=200||word(b+14)!=256)return false;
  for(unsigned i=16;i<256;++i)if(b[i])return false;
  for(unsigned i=0;i<256;++i){const int angle=signed_word(b+256+4*i);const unsigned scale=word(b+1280+2*i);
   if(angle<=-9000000||angle>=-7000000||!scale||scale>4096)return false;}
  data=b;return true;
 }
 int sample(unsigned offset,unsigned bytes,unsigned phase)const{
  const int at=int(phase*256)-32768,index=at>>16;
  const unsigned low=index<0?0:index>255?255:unsigned(index);
  const unsigned high=index+1<0?0:index+1>255?255:unsigned(index+1),fraction=unsigned(at)&65535u;
  const auto value=[&](unsigned i){const auto*p=data+offset+i*bytes;
   return bytes==4?signed_word(p):bytes==2?int(word(p)):int(*p);};
  const int a=value(low),b=value(high);
  return a+int(int64_t(b-a)*fraction/65536);
 }
};
struct Particle {int x,scale,angle;unsigned atlas,gray;};
// Original fixed_fps=0 follows desktop frame deltas. This bounded runtime
// uses the exact semi-implicit60Hz gravity sum and a continuous life clock;
// it does not claim identical GPU random histories or variable-step paths.
inline Particle particle(const Parameters&p,uint32_t elapsed,uint32_t seed,unsigned node,unsigned index){
 using namespace fake_merchant;
 const unsigned count=node<2?5:node<4?7:10,life=node<4?15000:30000;
 const uint64_t clock=uint64_t(elapsed)+100000-index*life/count;
 const unsigned age=unsigned(clock%life),cycle=unsigned(clock/life),phase=age*65536u/life;
 const uint32_t random=hash(seed^node*0x6c8e9cf5u^index*0x9e3779b9u^cycle*0x85ebca6bu);
 const unsigned steps=age*60u/1000u,gravity=node==2||node==3?5:15;
 const int x=int(uint64_t(gravity)*steps*(steps+1)*256/7200);
 const int initial=node==2||node==3?3*Q:4*Q+int(hash(random)%unsigned(2*Q+1));
 return {x,initial*p.sample(1280,2,phase)/Q,p.sample(256,4,phase)/256,
         hash(random+1)&3u,unsigned(p.sample(1792,1,phase))};
}
// Exact source alpha8 with clamped bilinear filtering. u/v use24fractional
// bits so affine step rounding stays below1/256 source texel across theLCD.
inline unsigned alpha(const uint8_t*data,int u,int v){
 const int x=int((int64_t(u)*200)>>16)-128,y=int((int64_t(v)*200)>>16)-128;
 const int ix=x>>8,iy=y>>8;const unsigned fx=unsigned(x)&255u,fy=unsigned(y)&255u;
 const unsigned a=ix<0?0:ix>199?199:unsigned(ix),b=ix+1<0?0:ix+1>199?199:unsigned(ix+1);
 const unsigned c=iy<0?0:iy>199?199:unsigned(iy),d=iy+1<0?0:iy+1>199?199:unsigned(iy+1);
 const unsigned top=data[c*200+a]*(256-fx)+data[c*200+b]*fx;
 const unsigned bottom=data[d*200+a]*(256-fx)+data[d*200+b]*fx;
 return (top*(256-fy)+bottom*fy+32768)>>16;
}
// `amount` is an already alpha-weighted RGB565 contribution. Separating
// constant source weighting from destination saturation is bit-exact.
inline uint16_t add_weighted(uint16_t destination,uint16_t amount){
 const unsigned r=(destination>>11)+(amount>>11);
 const unsigned g=((destination>>5)&63)+((amount>>5)&63);
 const unsigned b=(destination&31)+(amount&31);
 return uint16_t(((r>31?31:r)<<11)|((g>63?63:g)<<5)|(b>31?31:b));
}
}
