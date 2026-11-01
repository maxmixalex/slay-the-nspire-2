#include "tez_cinders_ui23.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire::tez_cinders_ui23;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"Tez cinders:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
struct Picture {const uint8_t*pixels;unsigned w,h,format;const void*alpha=nullptr,*palette=nullptr;};
static void put(uint8_t*p,uint32_t n,unsigned count=2){for(unsigned i=0;i<count;++i)p[i]=uint8_t(n>>(i*8));}
static std::vector<uint8_t> parameter_fixture(){
 std::vector<uint8_t>raw(26112);const unsigned header[]={0x5443,1,74,24,506,2,33,16667,FirstImage,EmitterOffset,DeltaOffset,CurveOffset,VelocityOffset,OrderOffset,Bytes,0};
 for(unsigned i=0;i<16;++i)put(raw.data()+i*2,header[i]);
 for(unsigned i=0;i<Emitters;++i){auto*r=raw.data()+EmitterOffset+i*32;
  put(r,65536,4);put(r+12,65536,4);put(r+16,32000,4);put(r+20,16000,4);
  r[24]=r[26]=uint8_t(i<2?i:255);r[25]=uint8_t(i<2);
 }
 for(unsigned i=0;i<768;++i)put(raw.data()+CurveOffset+i*4,i<256?i*1024:1048576,4);
 for(unsigned i=0;i<164;++i)put(raw.data()+OrderOffset+i*2,i<90?i:0x8000u|(i-90));
 // Two signed moving-parent velocity channels, without relying on alignment.
 put(raw.data()+VelocityOffset+4,0xff80);put(raw.data()+VelocityOffset+6,256);
 return raw;
}
static void table_contract(){
 auto raw=parameter_fixture();Table t;Picture p{raw.data(),256,51,0};CHECK(t.read(p));
 CHECK(t.coordinate(0,4,0)==32000);CHECK(t.command(90)==0x8000);CHECK(t.command(163)==0x8049);
 CHECK(t.inherit(0,-1).x==0&&t.inherit(2,1).y==0);CHECK(t.inherit(1,1).x==-32768); // first60Hz source interval
 for(uint32_t clock:{0u,33u,16666u,16667u,0xffffffffu})CHECK(t.frame(clock)==(clock%16667)/33);
 for(unsigned at:{0u,2u,8u,12u,16u,18u,30u,EmitterOffset+24,EmitterOffset+25,EmitterOffset+28,OrderOffset+2,Bytes}){
  raw[at]^=1;CHECK(!t.read(p));raw[at]^=1;CHECK(t.read(p));
 }
 put(raw.data()+CurveOffset,1048576,4);CHECK(!t.read(p));put(raw.data()+CurveOffset,0,4);CHECK(t.read(p));
 put(raw.data()+CurveOffset+256*4,0xffffffffu,4);CHECK(!t.read(p));
 auto unaligned=parameter_fixture();unaligned.insert(unaligned.begin(),0);CHECK(t.read(Picture{unaligned.data()+1,256,51,0}));
 CHECK(signed_bits(0x80000000u)==(-2147483647-1));CHECK(integer(unaligned.data()+1+EmitterOffset)==65536);
}
static void lifecycle(){
 auto raw=parameter_fixture();Table t;CHECK(t.read(Picture{raw.data(),256,51,0}));
 // A nonzero moving-emitter inheritance deliberately changes birth velocity.
 const auto inherit=[](unsigned emitter,int32_t born){return Velocity{int32_t((born%127)*(emitter+1)*256),-int32_t((born%43)*256)};};
 State stream,seek;unsigned polls=0;const auto poll=[&]{++polls;};
 for(uint32_t clock=0;clock<21000;clock+=33){
  stream.advance(clock,12345,t.source_curve(0),poll,inherit);
  if(clock%363==0){seek.valid=false;seek.advance(clock,12345,t.source_curve(0),poll,inherit);
   CHECK(std::memcmp(stream.particles,seek.particles,sizeof stream.particles)==0);}
 }
 for(uint32_t clock:{0u,33u,10000u,16667u,20000u,600000u,0xfffffff0u,10u}){
  stream.advance(clock,98765,t.source_curve(0),poll,inherit);seek.valid=false;seek.advance(clock,98765,t.source_curve(0),poll,inherit);
  CHECK(std::memcmp(stream.particles,seek.particles,sizeof stream.particles)==0);
  std::array<Particle,Particles>before;std::memcpy(before.data(),stream.particles,sizeof stream.particles);
  stream.advance(clock,98765,t.source_curve(0),poll,inherit);CHECK(std::memcmp(stream.particles,before.data(),sizeof stream.particles)==0);
 }
 CHECK(polls>Emitters);CHECK(sizeof(State)==56844);
 // Reduced rendering repeatedly requests clock zero, independent of UI time.
 stream.valid=false;stream.advance(0,27,t.source_curve(0),poll,inherit);
 seek=stream;for(unsigned i=0;i<9;++i)stream.advance(0,27,t.source_curve(0),poll,inherit);
 CHECK(std::memcmp(stream.particles,seek.particles,sizeof stream.particles)==0);
}
int main(){table_contract();lifecycle();std::printf("Cinder typed bounds, inheritance, stream/seek/wrap/pause: %u checks passed\n",checks);}
