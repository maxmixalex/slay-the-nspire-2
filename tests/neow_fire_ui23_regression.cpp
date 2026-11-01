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
 std::printf("Neow fire: %u checks, mask=%zu bytes, poll=%u\n",checks,sizeof(Mask),polls);
}
