#include "ancient_turbulence_ui23.hpp"
#include <vector>
#include <cstdio>
#include <cstdlib>
using namespace spire::ancient_turbulence_ui23;
static unsigned checks;
static void check(bool ok){++checks;if(!ok){std::fprintf(stderr,"trajectory check %u failed\n",checks);std::exit(1);}}
static void u16(std::vector<uint8_t>&b,unsigned p,unsigned v){b[p]=uint8_t(v);b[p+1]=uint8_t(v>>8);}
static void point(std::vector<uint8_t>&b,unsigned p,unsigned n,unsigned index,unsigned t,int x,int y){
 const int values[]={int(t),x,y};for(unsigned c=0;c<3;++c){b[p+c*2*n+index]=uint8_t(values[c]);b[p+(c*2+1)*n+index]=uint8_t(unsigned(values[c])>>8);}
}
static void u32(std::vector<uint8_t>&b,unsigned p,unsigned v){u16(b,p,v);u16(b,p+2,v>>16);}
int main(){
 const unsigned tex=576,ramp=580,dir=2628,track=2652,total=2676;
 std::vector<uint8_t>b(3072,0);b[0]='A';b[1]='T';b[2]='P';b[3]='1';u16(b,4,2);u16(b,6,2);
 u32(b,8,1000);u32(b,12,2000);u16(b,16,30);u16(b,20,2);u16(b,22,2);
 u32(b,24,tex);u32(b,28,ramp);u32(b,32,dir);u32(b,36,total);u32(b,40,512);u32(b,44,512);u32(b,56,64);
 for(unsigned i=64;i<576;++i)b[i]=255;for(unsigned i=0;i<4;++i)u16(b,48+i*2,65535);
 for(unsigned i=0;i<256;++i){for(unsigned j=0;j<4;++j)b[ramp+i*4+j]=uint8_t(i);u16(b,ramp+1024+i*2,i*257);u16(b,ramp+1536+i*2,4096);}
 for(unsigned i=0;i<2;++i){const unsigned r=dir+i*12,p=track+i*12;u32(b,r,p);u16(b,r+4,2);u16(b,r+6,4096);u16(b,r+8,128);
  point(b,p,2,0,0,1600+i*160,800);point(b,p,2,1,30,1760+i*160,640);}
 Table t;check(t.read(b.data(),unsigned(b.size())));check(t.count==2&&t.palette==64&&t.life==1000&&t.fps==30);
 check(t.ramp(0,0)==0&&t.ramp(0,65535)==255);check(t.ramp(4,0)==0&&t.ramp(4,65535)==65535);check(t.ramp(5,12345)==4096);
 for(uint32_t elapsed: {0u,1u,499u,500u,999u,1000u,30000u,0x7fffffffu,0xffffffffu})for(unsigned index=0;index<2;++index){
  Sample p;check(t.sample(index,elapsed,12345,p));
  const uint64_t time=uint64_t(elapsed)+3000-index*500;const unsigned age=unsigned(time%1000),cycle=unsigned(time/1000);
  const unsigned selected=(index+hash(12345^cycle*0x9e3779b9u)%2)%2;
  const unsigned subtick=unsigned(uint64_t(age)*30*256/1000);
  check(p.x==int((1600+selected*160)*16+uint64_t(160)*16*subtick/(30*256)));
  check(p.y==800*16-int(uint64_t(160)*16*subtick/(30*256)));
  check(p.phase==age*65536/1000&&p.scale==4096);Sample same;check(t.sample(index,elapsed,12345,same)&&same.x==p.x&&same.y==p.y);
 }
 Sample s;check(!t.sample(2,0,0,s));auto bad=b;u32(bad,24,0xfffffff0u);check(!t.read(bad.data(),unsigned(bad.size())));
 bad=b;u32(bad,36,30000);check(!t.read(bad.data(),unsigned(bad.size())));bad=b;u16(bad,6,101);check(!t.read(bad.data(),unsigned(bad.size())));
 bad=b;u32(bad,dir,0xffffff00u);u32(bad,dir+12,0xffffff00u);check(t.read(bad.data(),unsigned(bad.size()))&&!t.sample(0,0,12345,s));
 check(!t.read(b.data(),10));std::printf("%u native trajectory format/clock checks passed\n",checks);
}
