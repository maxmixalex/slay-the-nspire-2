#include <array>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "neow_fire_ui23.hpp"
using namespace spire::neow_fire_ui23;
static unsigned checks,polls;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"Neow fire:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void put16(uint8_t*p,unsigned q){p[0]=uint8_t(q);p[1]=uint8_t(q>>8);}
static void put32(uint8_t*p,uint32_t q){put16(p,q);put16(p+2,q>>16);}
struct Picture{const uint8_t*pixels,*alpha=nullptr,*palette=nullptr;unsigned w=256,h=1,format=0;};
struct Content{std::array<std::array<uint8_t,512>,Frames> bytes{};unsigned gen=1,loads=0;bool corrupt=false;
 unsigned generation(){return gen;}Picture image(int index){++loads;if(corrupt)return {nullptr};return {bytes[unsigned(index)-FirstMask].data()};}};
int main(){
 std::array<uint8_t,4608> raw{};auto*p=raw.data();memcpy(p,"NWF1",4);
 const unsigned h[]={1,1,4,Frames,KeyInterval,MaskBytes,FirstImage,TexturePages,FirstMask,ParameterImage,NodesAt,ModelsAt,RowsAt,SpansAt,Spans,Length};for(unsigned i=0;i<16;++i)put16(p+4+2*i,h[i]);put32(p+36,200000);put32(p+40,Duration);
 for(unsigned i=0;i<2;++i)put32(p+ModelsAt+i*128+124,1<<24);
 unsigned span=0,pixel=0;for(unsigned y=0;y<240;++y){put16(p+RowsAt+y*4,span);put16(p+RowsAt+y*4+2,pixel);for(unsigned n=0;n<(y<225?3u:2u);++n){unsigned count=span<565?25:24;put16(p+SpansAt+span*4,n*100);put16(p+SpansAt+span*4+2,count);++span;pixel+=count;}}
 CHECK(span==Spans&&pixel==MaskBytes);put16(p+RowsAt+240*4,span);put16(p+RowsAt+240*4+2,pixel);
 Picture picture{p,nullptr,nullptr,256,9,0};Table table;CHECK(table.read(picture,false));CHECK(!table.read(picture,true));put16(p+6,2);CHECK(table.read(picture,true));put16(p+6,1);
 for(unsigned at:{0u,4u,6u,8u,10u,12u,14u,16u,18u,20u,22u,24u,26u,28u,30u,32u,34u,36u,40u,44u,Length,SpansAt+2,RowsAt+4}){p[at]^=1;CHECK(!table.read(picture,false));p[at]^=1;}
 put32(p+NodesAt+8,0x7fffffffu);CHECK(!table.read(picture,false));put32(p+NodesAt+8,0);CHECK(table.read(picture,false));
 std::array<uint8_t,13312> v2{};memcpy(v2.data(),p,Length);put16(v2.data()+4,2);put16(v2.data()+34,Length+(MaskBytes+1)/2);put32(v2.data()+44,Length);put32(v2.data()+48,(MaskBytes+1)/2);memset(v2.data()+Length,255,(MaskBytes+1)/2);v2[Length+(MaskBytes+1)/2-1]=15;
 Picture bound_picture{v2.data(),nullptr,nullptr,256,26,0};CHECK(table.read(bound_picture,false));for(unsigned n=0;n<4;++n)for(unsigned i=0;i<MaskBytes;++i)CHECK(table.active(n,i));
 v2[Length]=0x21;CHECK(table.read(bound_picture,false));CHECK(table.active(0,0)&&!table.active(1,0)&&!table.active(0,1)&&table.active(1,1));v2[Length]=255;
 for(unsigned at:{44u,48u,Length+(MaskBytes+1)/2}){v2[at]^=1;CHECK(!table.read(bound_picture,false));v2[at]^=1;}v2[Length+(MaskBytes+1)/2-1]|=16;CHECK(!table.read(bound_picture,false));v2[Length+(MaskBytes+1)/2-1]=15;put16(v2.data()+4,3);CHECK(!table.read(bound_picture,false));CHECK(table.read(picture,false));
 Content content;for(unsigned frame=0;frame<Frames;++frame){auto*r=content.bytes[frame].data();put32(r,13);put16(r+4,frame);put16(r+6,1);put16(r+8,frame%MaskBytes);put16(r+10,0x8001);r[12]=uint8_t(frame+1);}
 Mask mask;auto poll=[](){++polls;};CHECK(mask.restore(content,FirstMask,31,poll));CHECK(content.loads==32&&polls>0);for(unsigned i=0;i<32;++i)CHECK(mask.pixels[i]==uint8_t(i+1));
 CHECK(mask.restore(content,FirstMask,31,poll));CHECK(content.loads==32);CHECK(mask.restore(content,FirstMask,33,poll));CHECK(content.loads==34);CHECK(mask.pixels[0]==0&&mask.pixels[32]==33&&mask.pixels[33]==34);
 CHECK(mask.restore(content,FirstMask,545,poll));CHECK(content.loads==36);CHECK(mask.restore(content,FirstMask,0,poll));CHECK(content.loads==37);CHECK(mask.pixels[0]==1&&mask.pixels[545]==0);
 content.gen++;CHECK(mask.restore(content,FirstMask,0,poll));CHECK(content.loads==38);content.corrupt=true;CHECK(!mask.restore(content,FirstMask,1,poll)&&!mask.valid);content.corrupt=false;CHECK(mask.restore(content,FirstMask,1,poll));CHECK(content.loads==41);
 CHECK(!mask.restore(content,FirstMask,546,poll));auto bad=content.bytes[0];Picture bp{bad.data()};put32(bad.data(),7);CHECK(!mask.apply(bp,0,poll));put32(bad.data(),13);put16(bad.data()+10,0xffff);CHECK(!mask.apply(bp,0,poll));
 // Constant original channels make the independent shader-clock property
 // visible without embedding source art. Q20 sampling stays defined at wrap.
 uint8_t rgb[]={127,127,127};spire::ancient_fire_ui23::Field fields[4];for(auto&f:fields){f.width=f.height=1;f.channels=3;f.pages[0]=rgb;}
 auto*m=p+ModelsAt;for(unsigned i=0;i<4;++i){put32(m+i*20,1<<24);put32(m+i*20+4,1<<24);put32(m+i*20+16,1<<24);}for(unsigned i:{20u,21u,24u,25u})put32(m+i*4,1<<24);
 Shader shader;for(uint32_t clock:{0u,33u,17999u,18000u,199999u,200000u,0xffffffffu}){shader.bind(fields,m,clock);for(int u=-100000;u<=1100000;u+=33333)for(int v=-50000;v<=1100000;v+=55555)CHECK(shader.alpha(u,v)<=1048576);}
 // Eligible unit/half-unit models must exactly match the general equation,
 // including zero/saturated bounds and UVs outside the specialized domain.
 std::array<uint8_t,257*300*3> texture{};uint32_t rng=73;
 for(auto&v:texture){rng=rng*1664525u+1013904223u;v=uint8_t(rng>>24);}
 for(auto&f:fields){f.width=257;f.height=300;f.channels=3;f.pages[0]=texture.data();f.pages[1]=texture.data()+131072;}
 for(unsigned half=0;half<2;++half){
  for(unsigned i:{0u,1u,5u,6u})put32(m+i*4,half?8388608:16777216);
  put32(m+21*4,33554432);put32(m+14*4,1593836);put32(m+19*4,1677722);put32(m+25*4,20132659);
  put32(m+30*4,4781507);put32(m+31*4,16609444);
  for(uint32_t clock:{0u,33u,199999u,200000u,0xffffffffu}){
   shader.bind(fields,m,clock);CHECK(shader.inverse&0x80000000u);Shader general=shader;general.inverse&=0x3fffffffu;
   for(unsigned i=0;i<2000;++i){rng=rng*1664525u+1013904223u;int u=int(rng&2097151u)-524288;rng=rng*1664525u+1013904223u;int v=int(rng&2097151u)-524288;CHECK(shader.alpha(u,v)==general.alpha(u,v));}
  }
 }
 // Every condition of the fast-path proof is checked once at bind, and an
 // otherwise valid different scale must fall back instead of being rounded.
 for(unsigned i:{0u,1u,4u,5u,6u,9u,10u,11u,15u,16u,20u,21u,24u}){const unsigned old=spire::ancient_fire_ui23::dword(m+i*4);put32(m+i*4,old+1);shader.bind(fields,m,0);CHECK(!(shader.inverse&0x80000000u));put32(m+i*4,old);}
 // Clamped zero-edge elision must prove all four edges, then preserve every
 // interior/border sample. A changed source edge disables it immediately.
 std::vector<uint8_t> pinned(600*636*3,127);
 const unsigned dimensions[][3]={{600,636,1},{250,250,3},{256,256,3},{100,100,2}};
 for(unsigned i=0;i<4;++i){fields[i].width=dimensions[i][0];fields[i].height=dimensions[i][1];fields[i].channels=dimensions[i][2];
  for(unsigned page=0;page<8;++page)fields[i].pages[page]=pinned.data()+page*131072;}
 for(unsigned i=0;i<256;++i)for(unsigned at:{i*3+2,(255*256+i)*3+2,i*256*3+2,(i*256+255)*3+2})pinned[at]=0;
 shader.bind(fields,m,0);CHECK(shader.lcd_sampling&&shader.empty_light_border);
 for(int v=-524288;v<1572864;v+=15683)for(int u=-524288;u<1572864;u+=7919)
  CHECK(shader.sample(2,u,v,2,false)==fields[2].sample_lcd(u,v,2,false));
 for(unsigned at:{128*3+2,(255*256+128)*3+2,128*256*3+2,(128*256+255)*3+2}){
  pinned[at]=1;shader.bind(fields,m,0);CHECK(!shader.empty_light_border);pinned[at]=0;
 }
 std::printf("Neow fire: %u checks, mask=%zu bytes, poll=%u\n",checks,sizeof(Mask),polls);
}
