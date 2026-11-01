#include "ancient_gpu_masks_ui23.hpp"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"Ancient GPU:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){
 using namespace ancient_gpu_masks_ui23;
 std::array<uint8_t,1024> bytes{};
 for(unsigned i=0;i<256;++i)for(unsigned c=0;c<4;++c)bytes[i*4+c]=uint8_t(i*37+c*23);
 for(unsigned phase=0;phase<=65536;++phase){
  const double x=phase/65536.0*256-.5;const int left=int(std::floor(x));const double f=x-left;
  const auto clamp=[](int i){return i<0?0:i>255?255:i;};
  for(unsigned c=0;c<4;++c){const double value=bytes[clamp(left)*4+c]*(1-f)+bytes[clamp(left+1)*4+c]*f;
   CHECK(std::abs(int(ramp(bytes.data(),4,c,phase))-value)<=1.5);}
 }
 for(uint32_t clock:{0u,1u,7999u,8000u,9999u,10000u,0xffffffffu}){
  CHECK(light_phase(clock)==((uint64_t(clock)+2000)%10000)*65536/10000);
  for(unsigned n=0;n<5;++n)for(unsigned i=0;i<40;++i){
   const auto a=sparkle(clock,0xabcdef98,n,i,40),b=sparkle(clock,0xabcdef98,n,i,40);
   CHECK(a.x==b.x&&a.y==b.y&&a.age==b.age&&a.scale==b.scale);
   CHECK(a.age<5000&&a.scale>=fake_merchant::fixed(.1)&&a.scale<=fake_merchant::fixed(.3));
   CHECK(int64_t(a.x)*a.x+int64_t(a.y)*a.y<=(int64_t(102.2*256)*int64_t(102.2*256)));
  }
 }
 const auto first=sparkle(0,42,0,0,20),last=sparkle(0,42,0,19,20);
 CHECK(first.age==0&&last.age==250);
 std::printf("Ancient GPU masks: %u checks; gradient sampling bound, independent clocks, deterministic sphere-volume poses\n",checks);
}
