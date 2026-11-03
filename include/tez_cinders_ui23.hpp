#pragma once
#include <stdint.h>
#include "combat_card_motion.hpp"

namespace spire::tez_cinders_ui23 {
// 74 original emitters *24 particles, in the existing scene-owned union.
// Local Q16 physics, 60Hz. No persistent global particle allocation.
constexpr unsigned Emitters=74,PerEmitter=24,Particles=Emitters*PerEmitter;
#ifndef SPIRE_TEZ_BACKGROUND_STRIDE
#define SPIRE_TEZ_BACKGROUND_STRIDE 12
#endif
// Keep both foreground emitters complete. Background decoration selects a
// staggered subset of original particle indices, never a slower simulation.
constexpr unsigned CalculatorBackgroundStride=SPIRE_TEZ_BACKGROUND_STRIDE;
static_assert(CalculatorBackgroundStride>=1&&CalculatorBackgroundStride<=PerEmitter,"Bounded background particle sampling");
inline unsigned presentation_step(unsigned emitter){return emitter<2?1:CalculatorBackgroundStride;}
inline unsigned presentation_first(unsigned emitter){return emitter<2?0:emitter%CalculatorBackgroundStride;}
struct Particle {
 int32_t x,y,vx,vy;
 uint16_t age_half,life64;
 int16_t orbit8192;uint16_t scale4096;
 uint8_t red,green,blue,active;
 int16_t orientation;uint16_t reserved;
};
static_assert(sizeof(Particle)==32,"Cinder state must retain its bounded scene-union layout");
struct SinCos {int32_t sine,cosine;};
inline int32_t signed_bits(uint32_t n){return n<=0x7fffffffu?int32_t(n):-1-int32_t(~n);}
inline uint32_t hash(uint32_t n){n=((n>>16)^n)*73244475u;n=((n>>16)^n)*73244475u;return (n>>16)^n;}
inline unsigned random(uint32_t&seed){
 int32_t s=signed_bits(seed);if(!s)s=305420679;
 const int32_t k=s/127773;s=16807*(s-k*127773)-2836*k;if(s<0)s+=2147483647;
 seed=uint32_t(s);return seed&65535u;
}
// Q28 radians -> Q30 sine/cosine. Bounded CORDIC is only needed on birth.
inline SinCos sincos(int32_t angle){
 static constexpr int32_t at[]={210828714,124459457,65760959,33381290,16755422,8385879,4193963,2097109,1048571,524287,262144,131072,65536,32768,16384,8192,4096,2048,1024,512,256,128};
 constexpr int32_t Pi=843314857,HalfPi=421657428;
 bool flip=false;if(angle>HalfPi){angle-=Pi;flip=true;}else if(angle<-HalfPi){angle+=Pi;flip=true;}
 int32_t x=652032874,y=0;
 for(unsigned i=0;i<22;++i){const int32_t old=x;if(angle>=0){x-=y>>i;y+=old>>i;angle-=at[i];}else{x+=y>>i;y-=old>>i;angle+=at[i];}}
 return {flip?-y:y,flip?-x:x};
}
inline unsigned square_root(unsigned value){unsigned result=0,bit=1u<<30;while(bit>value)bit>>=2;
 while(bit){if(value>=result+bit){value-=result+bit;result=(result>>1)+bit;}else result>>=1;bit>>=2;}return result;}
inline int32_t round_div(int64_t n,int32_t d){return int32_t(n>=0?(n+d/2)/d:-((-n+d/2)/d));}
template<class Curve> inline int32_t curve(Curve values,unsigned q){
 const int32_t pos=int32_t(q*256u)-32768;int index=pos>>16;const unsigned f=unsigned(pos)&65535u;
 const int a=index<0?0:index>255?255:index,b=index+1<0?0:index+1>255?255:index+1;
 return int32_t((int64_t(values[a])*(65536-f)+int64_t(values[b])*f+32768)>>16);
}
// The division branch has n<life64<=65535, so its Q16 numerator fits uint32.
inline unsigned age_q16(const Particle&p){const uint32_t n=uint32_t(p.age_half)*32u;return n>=p.life64?65536u:n*65536u/p.life64;}
inline void hue(Particle&p,unsigned random_value){
 const int32_t angle=round_div((int64_t(random_value)*7-3*65535)*1686628291ll,100*65535);
 const auto sc=sincos(angle);
 // Original Godot shader hue matrix applied to source(.772549,.54902,.239216).
 static constexpr int32_t base[]={489344,960685,186573};
 static constexpr int32_t cosine[]={320482,-385247,64264};
 static constexpr int32_t sine[]={260812,24096,-285426};
 for(unsigned i=0;i<3;++i){const int32_t q=base[i]+int32_t((int64_t(cosine[i])*sc.cosine+int64_t(sine[i])*sc.sine)>>30);
  const int value=round_div(int64_t(q)*255,1048576);const uint8_t channel=uint8_t(value<0?0:value>255?255:value);
  if(!i)p.red=channel;else if(i==1)p.green=channel;else p.blue=channel;}
}
inline void birth(Particle&p,bool sphere,uint32_t seed){
 seed=hash(seed+1);p={};
 const unsigned size=random(seed),hue_value=random(seed);random(seed);random(seed);
 const unsigned life=random(seed);p.life64=uint16_t(int64_t(sphere?600:420)*64*(131070-life)/131070);
 p.scale4096=uint16_t(sphere?4096+round_div(int64_t(size)*4096,5*65535):2048+round_div(int64_t(size)*4096,5*65535));
 random(seed);random(seed);const unsigned speed=random(seed);random(seed);random(seed);
 p.orbit8192=int16_t(-8192+round_div(int64_t(random(seed))*32768,65535));
 random(seed);random(seed);random(seed);random(seed);random(seed); // physics and amount_ratio
 if(sphere){
  const int32_t s=round_div(int64_t(random(seed))*65536,65535)-32768;
  int32_t a=round_div(int64_t(random(seed))*1686628291ll,65535);if(a>843314857)a-=1686629714;
  const unsigned radius=random(seed);const unsigned projected=square_root((1u<<30)-unsigned(int64_t(s)*s));
  const auto sc=sincos(a);const int32_t radial=round_div(int64_t(projected)*radius*100,65535);
  p.x=int32_t(int64_t(radial)*sc.cosine>>30);p.y=int32_t(int64_t(radial)*sc.sine>>30);
 }else{
  p.x=round_div((int64_t(random(seed))*2-65535)*20*65536,65535);
  p.y=round_div((int64_t(random(seed))*2-65535)*20*65536,65535);random(seed);
 }
 const int32_t angle=-421657428+round_div((int64_t(random(seed))*2-65535)*74961321,65535);
 const auto sc=sincos(angle);const int32_t velocity=100*65536+round_div(int64_t(speed)*100*65536,65535);
 p.vx=int32_t(int64_t(velocity)*sc.cosine>>30);p.vy=int32_t(int64_t(velocity)*sc.sine>>30);
 hue(p,hue_value);p.active=1;p.orientation=int16_t(CombatCardMotion::direction(p.vy>>8,p.vx>>8)-1024);
}
// Original orbit is a position rotation divided by DELTA, then integrated as
// part of final_velocity. Local coordinates make EMISSION_TRANSFORM identity.
// Gravity updates stored velocity before the position update. The tiny orbit
// angle (abs<.014 rad/full tick) permits bounded Q30 Taylor terms.
template<class Curve> inline void step(Particle&p,Curve orbit_curve,unsigned half_ticks=2){
 p.age_half=uint16_t(p.age_half+half_ticks);if(uint32_t(p.age_half)*32u>p.life64){p.active=0;return;}
 const int32_t value=curve(orbit_curve,age_q16(p));
 int32_t angle=0;
 if(p.orbit8192<=-82||p.orbit8192>=82)
  angle=int32_t((int64_t(p.orbit8192)*value*112441981ll)>>33)*int(half_ticks)/2;
 const int32_t aa=int32_t(int64_t(angle)*angle>>30);
 const int32_t sine=angle-int32_t(int64_t(angle)*aa/6>>30),cosine=(1<<30)-aa/2+int32_t(int64_t(aa)*aa/24>>30);
 const int32_t x=int32_t((int64_t(p.x)*cosine+int64_t(p.y)*sine)>>30);
 const int32_t y=int32_t((-int64_t(p.x)*sine+int64_t(p.y)*cosine)>>30);
 p.vx+=round_div(int64_t(100)*65536*half_ticks,120);p.vy+=round_div(-int64_t(20)*65536*half_ticks,120);
 const int32_t nx=x+round_div(int64_t(p.vx)*half_ticks,120),ny=y+round_div(int64_t(p.vy)*half_ticks,120);
 p.orientation=int16_t(CombatCardMotion::direction((ny-p.y)>>4,(nx-p.x)>>4)-1024);p.x=nx;p.y=ny;
}

struct Velocity {int32_t x=0,y=0;};
struct NoInheritance {Velocity operator()(unsigned,int32_t)const{return {};}};
constexpr unsigned AuxiliaryFirst=7221,AuxiliaryCount=7222,FirstImage=59710,Images=59712;
constexpr unsigned EmitterOffset=32,DeltaOffset=2400,CurveOffset=14544,VelocityOffset=17616,OrderOffset=25616,Bytes=25944;
inline unsigned word(const uint8_t*p){return unsigned(p[0])|unsigned(p[1])<<8;}
inline int signed_word(const uint8_t*p){const unsigned n=word(p);return n<32768?int(n):int(n)-65536;}
inline int32_t integer(const uint8_t*p){return signed_bits(word(p)|(uint32_t(word(p+2))<<16));}
struct SourceCurve {const uint8_t*data;int32_t operator[](unsigned i)const{return integer(data+i*4);}};
struct Table {
 const uint8_t*data=nullptr;
 template<class Picture> bool read(Picture p){
  data=nullptr;if(!p.pixels||p.w!=256||p.h!=51||p.format||p.alpha||p.palette)return false;
  static constexpr uint16_t expected[]={0x5443,1,74,24,506,2,33,16667,FirstImage,EmitterOffset,DeltaOffset,CurveOffset,VelocityOffset,OrderOffset,Bytes,0};
  for(unsigned i=0;i<16;++i)if(word(p.pixels+i*2)!=expected[i])return false;
  unsigned light=0,cinder=0;
  for(unsigned i=0;i<164;++i){const unsigned n=word(p.pixels+OrderOffset+i*2);if(n&0x8000){if(n!=(0x8000u|cinder++))return false;}else if(n!=light++)return false;}
  if(light!=90||cinder!=74)return false;
  for(unsigned i=0;i<74;++i){const auto*r=p.pixels+EmitterOffset+i*32;
   for(unsigned j=0;j<6;++j)if(integer(r+j*4)<=-8388608||integer(r+j*4)>=8388608)return false;
   if(r[24]!=(i<2?i:255)||r[25]!=(i<2)||r[26]!=(i<2?i:255)||r[27]||integer(r+28))return false;
  }
  for(unsigned i=0;i<768;++i){const int32_t value=integer(p.pixels+CurveOffset+i*4);
   if(i<256?(value<=-1048576||value>=1048576):(value<0||value>1048576))return false;
  }
  for(unsigned i=Bytes;i<256u*51u*2u;++i)if(p.pixels[i])return false;
  data=p.pixels;return true;
 }
 SourceCurve source_curve(unsigned index)const{return {data+CurveOffset+index*1024};}
 unsigned frame(uint32_t clock)const{return (clock%16667)/33;}
 int32_t coordinate(unsigned emitter,unsigned component,unsigned frame)const{
  const auto*r=data+EmitterOffset+emitter*32;int32_t value=integer(r+component*4);
  if(emitter<2)value+=signed_word(data+DeltaOffset+(emitter*506+frame)*12+component*2);
  return value;
 }
 Velocity inherit(unsigned emitter,int32_t birth_half)const{
  if(emitter>=2||birth_half<=0)return {};
  const uint32_t ms=uint32_t(uint64_t((uint32_t(birth_half)+1)/2)*1000/60)%16667;
  unsigned index=ms*60/1000;if(index>=1000)index=999;
  const auto*p=data+VelocityOffset+(index*2+emitter)*4;
  return {signed_word(p)*256,signed_word(p+2)*256};
 }
 unsigned command(unsigned index)const{return word(data+OrderOffset+index*2);}
};
struct State {
 Particle particles[Particles];
 uint32_t tick=0,seed=0;bool valid=false,box_mode=false;uint8_t density_stride=1;
 // The caller binds content/language/room identity in the scene union owner.
 // Ordinary frames only advance newly elapsed fixed ticks. Reopen/seek uses
 // at most the current source lifecycle, never all prior room history.
 template<class Curve,class Poll,class Inherit=NoInheritance> void advance(uint32_t elapsed,uint32_t cosmetic,Curve orbit_curve,Poll poll,Inherit inherit={},bool box_only=false,unsigned stride=1){
  if(!stride||stride>PerEmitter)stride=1;
  const uint32_t target=uint32_t(uint64_t(elapsed)*60/1000)+360u;
  const bool reset=!valid||box_mode!=box_only||density_stride!=stride||seed!=cosmetic||target<tick||target-tick>600;
  for(unsigned emitter=0;emitter<(box_only?1u:Emitters);++emitter){
   poll();const bool sphere=!box_only&&emitter<2;const unsigned period=sphere?600:420;
   const uint32_t emitter_seed=hash(cosmetic+emitter*0x9e3779b9u);
   const unsigned spacing=emitter<2?1:stride;
   for(unsigned j=emitter<2?0:emitter%stride;j<PerEmitter;j+=spacing){auto&p=particles[emitter*PerEmitter+j];
    const unsigned phase=period*2*j/PerEmitter;const uint64_t now=uint64_t(target)*2;
    if(now<phase){p={};continue;}
    const uint64_t local=now-phase;const unsigned age=unsigned(local%(period*2));
    const uint32_t cycle=uint32_t(local/(period*2));
    const uint64_t prior=uint64_t(tick)*2;
    const bool restart=reset||prior<phase||uint32_t((prior-phase)/(period*2))!=cycle;
    if(restart){birth(p,sphere,emitter_seed+cycle*PerEmitter+j);
     const int32_t born=int32_t(uint64_t(cycle)*period*2+phase)-720;
     if(sphere&&born>0){const auto v=inherit(emitter,born);p.vx+=v.x;p.vy+=v.y;}
     if(age&1)step(p,orbit_curve,1);}
    while(p.age_half<age&&p.active)step(p,orbit_curve,2);
    if(!p.active)p.age_half=uint16_t(age);
   }
  }
  seed=cosmetic;tick=target;box_mode=box_only;density_stride=uint8_t(stride);valid=true;
 }
};
static_assert(sizeof(State)==56844,"Complete cinder state must retain its reviewed scene-union tail");
}
