#pragma once
#include <stdint.h>
#include "fake_merchant_presentation.hpp"
namespace spire::ancient_gpu_masks_ui23 {
constexpr unsigned AuxiliaryFirst=7220,AuxiliaryCount=7221,FirstImage=59704,Images=59714;
constexpr unsigned ParameterBytes=1536,MaskCount=8;
inline unsigned word(const uint8_t*p){return p[0]|unsigned(p[1])<<8;}
struct Mask {unsigned image,x,y,w,h,stride;};
struct Parameters {
 const uint8_t*data=nullptr;unsigned first=0,dot=0;
 template<class Picture> bool read(Picture p){
  if(!p.pixels||p.w!=256||p.h!=3||p.format||p.alpha||p.palette)return false;
  const auto*b=p.pixels;if(b[0]!='A'||b[1]!='G'||b[2]!='M'||b[3]!='1'||word(b+4)!=1||word(b+6)!=8||word(b+12)!=320||word(b+14)!=240)return false;
  first=word(b+8);dot=word(b+10);if(first!=FirstImage||dot!=first+8)return false;
  data=b;for(unsigned i=0;i<8;++i){const auto r=mask(i);
   if(r.image!=first+i||!r.w||!r.h||r.x+r.w>320||r.y+r.h>240||r.stride!=((r.w+1)&~1u)){data=nullptr;return false;}}
  for(unsigned i=112;i<128;++i)if(b[i]){data=nullptr;return false;}
  for(unsigned i=1408;i<1536;++i)if(b[i]){data=nullptr;return false;}return true;
 }
 Mask mask(unsigned i)const{const auto*p=data+16+i*12;return {word(p),word(p+2),word(p+4),word(p+6),word(p+8),word(p+10)};}
};
// Godot's generated256-sample gradient textures are kept as original bytes.
// Clamp/bilinear semantics use normalized texture-center coordinates.
inline unsigned ramp(const uint8_t*data,unsigned channels,unsigned channel,unsigned phase){
 const int x=int(phase*256)-32768,i=x>>16;const unsigned f=unsigned(x&65535)>>8;
 const unsigned a=unsigned(i<0?0:i>255?255:i),b=unsigned(i+1<0?0:i+1>255?255:i+1);
 return (data[a*channels+channel]*(256-f)+data[b*channels+channel]*f+128)>>8;
}
constexpr unsigned light_phase(uint32_t elapsed){return (elapsed%10000u+2000u)%10000u*65536u/10000u;}
struct Particle {int x,y,scale;unsigned age;};
inline Particle sparkle(uint32_t elapsed,uint32_t seed,unsigned node,unsigned index,unsigned count){
 using namespace fake_merchant;
 const uint64_t time=uint64_t(elapsed)+5000u-index*5000u/count;
 const unsigned age=unsigned(time%5000u),cycle=unsigned(time/5000u);
 const uint32_t h=hash(seed^node*0x6c8e9cf5u^index*0x9e3779b9u^cycle*0x85ebca6bu);
 // Godot4.5.1 ParticleProcessMaterial sphere-volume generation: random
 // latitude s, azimuth t and radial p; source disable_z discards only z.
 const int s=int(hash(h)&8191u)-4096;
 const unsigned radial=root(unsigned(Q*Q-s*s));
 const unsigned angle=hash(h+1)&4095u,p=hash(h+2)&65535u;
 const int radius=int(int64_t(fixed(102.1))*radial*p/(int64_t(Q)*65535));
 return {int(int64_t(radius)*CombatCardMotion::sine(int(angle)+1024)/(1024*16)),
         int(int64_t(radius)*CombatCardMotion::sine(int(angle))/(1024*16)),
         fixed(.1)+int(hash(h+3)%unsigned(fixed(.3)-fixed(.1)+1)),age};
}
}
