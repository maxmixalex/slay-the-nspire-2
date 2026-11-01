#include "nonu_fog_ui23.hpp"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <iterator>
using namespace spire::nonu_fog_ui23;
struct Picture {const uint8_t*pixels;int w,h,format;const uint8_t*alpha,*palette;};
int main(int argc,char**argv){
 assert(argc==3);std::ifstream table(argv[1],std::ios::binary),mask(argv[2],std::ios::binary);
 std::vector<uint8_t>bytes((std::istreambuf_iterator<char>(table)),{}),pixels((std::istreambuf_iterator<char>(mask)),{});
 assert(bytes.size()==2048&&pixels.size()==40000);Parameters p;
 assert(p.read(Picture{bytes.data(),256,4,0,nullptr,nullptr}));unsigned checks=1;
 auto invalid=bytes;invalid[8]^=1;assert(!Parameters{}.read(Picture{invalid.data(),256,4,0,nullptr,nullptr}));++checks;
 invalid=bytes;invalid[255]=1;assert(!Parameters{}.read(Picture{invalid.data(),256,4,0,nullptr,nullptr}));++checks;
 invalid=bytes;invalid[1280]=invalid[1281]=0;assert(!Parameters{}.read(Picture{invalid.data(),256,4,0,nullptr,nullptr}));++checks;
 for(unsigned node=0;node<6;++node){const unsigned count=node<2?5:node<4?7:10,life=node<4?15000:30000;
  for(unsigned age=0;age<life;age+=7){const unsigned n=age*60/1000,g=node==2||node==3?5:15;
   double v=0,x=0;for(unsigned i=0;i<n;++i){v+=double(g)/60;x+=v/60;}
   const int actual=int(uint64_t(g)*n*(n+1)*256/7200);
   assert(fabs(actual/256.-x)<1./256.+1e-7);++checks;
  }
  for(uint32_t clock:{0u,1u,33u,1000u,14999u,15000u,29999u,30000u,100000u,0xfffffff0u,0xffffffffu})
   for(unsigned index=0;index<count;++index){const auto a=particle(p,clock,0x12345678u,node,index);
    const auto b=particle(p,clock,0x12345678u,node,index);
    assert(a.x==b.x&&a.angle==b.angle&&a.scale==b.scale&&a.atlas==b.atlas&&a.gray==b.gray);
    assert(a.atlas<4&&a.gray<=255&&a.scale>0&&a.scale<=6*4096&&a.x>=0);checks+=2;
   }
 }
 double maximum=0;
 for(unsigned v=0;v<16777216u;v+=7919)for(unsigned u=0;u<16777216u;u+=123457){
  const double x=double(u)*200/16777216-.5,y=double(v)*200/16777216-.5;
  const int ix=int(floor(x)),iy=int(floor(y));const double fx=x-ix,fy=y-iy;
  const auto clip=[](int a){return a<0?0:a>199?199:a;};
  const double a=pixels[clip(iy)*200+clip(ix)]*(1-fx)+pixels[clip(iy)*200+clip(ix+1)]*fx;
  const double b=pixels[clip(iy+1)*200+clip(ix)]*(1-fx)+pixels[clip(iy+1)*200+clip(ix+1)]*fx;
  const double error=fabs(double(alpha(pixels.data(),u,v))-(a*(1-fy)+b*fy));
  if(error>maximum)maximum=error;assert(error<=1.5);++checks;
 }
 printf("{\"checks\":%u,\"max_source_alpha_error\":%.9f,\"variable_gpu_timestep_exact\":false}\n",checks,maximum);
}
