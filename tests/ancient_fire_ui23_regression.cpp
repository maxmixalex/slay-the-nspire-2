#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "ancient_fire_ui23.hpp"
using namespace spire::ancient_fire_ui23;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"Fire:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void put16(uint8_t*p,unsigned q){p[0]=uint8_t(q);p[1]=uint8_t(q>>8);}
static void put32(uint8_t*p,uint32_t q){put16(p,q);put16(p+2,q>>16);}
struct Picture {const uint8_t*pixels;unsigned w,h,format;};
int main(){
 std::array<uint8_t,((Length+639)/640)*640> raw{};auto*p=raw.data();
 p[0]='A';p[1]='F';p[2]='R';p[3]='1';
 const unsigned half[]={1,Nodes,Frames,Dynamic,Models,Commands};for(unsigned i=0;i<6;++i)put16(p+4+2*i,half[i]);
 const unsigned offsets[]={NodesAt,ModelsAt,DeltasAt,OrderAt,Length};for(unsigned i=0;i<5;++i)put32(p+16+4*i,offsets[i]);
 put16(p+36,FirstImage);put16(p+38,PageCount);put32(p+40,20000);put32(p+44,16667);
 for(unsigned i=0;i<Nodes;++i){auto*r=p+NodesAt+i*40;put32(r,10240);put32(r+4,20480);put32(r+8,2048);put32(r+20,4096);r[36]=uint8_t(i%3);r[37]=uint8_t(i%2);r[38]=uint8_t(i<Dynamic?i:255);}
 for(unsigned i=0;i<Models;++i){put32(p+ModelsAt+i*128+116,1<<24);put32(p+ModelsAt+i*128+124,1<<24);}
 unsigned command=0;for(unsigned k=0;k<3;++k)for(unsigned i=0;i<(k==0?90:k==1?74:237);++i)put16(p+OrderAt+2*command++,(k<<14)|i);
 for(unsigned n=0;n<Dynamic;++n)for(unsigned f=0;f<Frames;++f)put16(p+DeltasAt+(n*Frames+f)*12,unsigned(-int(f)));
 Picture picture{p,320,(Length+639)/640,0};Table table;CHECK(table.read(picture));
 for(unsigned n=0;n<Nodes;++n)for(unsigned f=0;f<Frames;++f)CHECK(table.coordinate(n,0,f)==10240-(n<Dynamic?int(f):0));
 for(uint32_t clock:{0u,33u,16666u,16667u,20000u,0xffffffffu})CHECK(table.frame(clock)==(clock%16667)/33);
 for(unsigned at:{0u,4u,6u,8u,10u,12u,14u,16u,20u,24u,28u,32u,36u,38u,40u,44u,52u,NodesAt+38,OrderAt+2,Length}){p[at]^=1;CHECK(!table.read(picture));p[at]^=1;}
 put32(p+48,1);CHECK(table.read(picture));put32(p+48,2);CHECK(!table.read(picture));put32(p+48,0);
 put32(p+ModelsAt+116,65535);CHECK(!table.read(picture));put32(p+ModelsAt+116,1<<24);CHECK(table.read(picture));
 picture.h=256;CHECK(!table.read(picture));picture.h=(Length+639)/640;
 // Independent double oracle exercises clamp/repeat, half-texel phase,
 // negative UVs, and page crossings without original-game assets.
 std::array<uint8_t,2*PageBytes> bytes{};for(unsigned i=0;i<bytes.size();++i)bytes[i]=uint8_t((i*73u+(i/517u)*31u)&255);
 Field field;field.width=517;field.height=300;field.channels=1;field.pages[0]=bytes.data();field.pages[1]=bytes.data()+PageBytes;
 uint32_t random=17;
 for(unsigned i=0;i<20000;++i){random=random*1664525u+1013904223u;int u=int(random&0x7fffff)-4194304;random=random*1664525u+1013904223u;int v=int(random&0x7fffff)-4194304;bool repeat=i&1;
  auto coordinate=[&](int a,unsigned size){double q=double(a)/1048576;if(repeat)q-=std::floor(q);else q=std::fmax(0,std::fmin(1,q));return q*size-.5;};
  double x=coordinate(u,field.width),y=coordinate(v,field.height);int ix=int(std::floor(x)),iy=int(std::floor(y));double fx=x-ix,fy=y-iy;
  auto pixel=[&](int px,int py){if(repeat){px=(px+int(field.width))%int(field.width);py=(py+int(field.height))%int(field.height);}else{px=bound(px,0,field.width-1);py=bound(py,0,field.height-1);}return bytes[py*field.width+px];};
  const double exact=((pixel(ix,iy)*(1-fx)+pixel(ix+1,iy)*fx)*(1-fy)+(pixel(ix,iy+1)*(1-fx)+pixel(ix+1,iy+1)*fx)*fy)/255*1048576;
  CHECK(std::abs(field.sample(u,v,0,repeat)-exact)<.51);
 }
 for(unsigned value=0;value<=255;++value){uint8_t byte=uint8_t(value);Field constant;constant.width=constant.height=constant.channels=1;constant.pages[0]=&byte;CHECK(std::abs(constant.sample(-100,200,0,true)-double(value)*1048576/255)<.51);}
 // Exact pre-optimization integer oracle, separate from the floating source
 // filter check. RGB texels deliberately straddle 128KiB cache pages.
 Field rgb;rgb.width=257;rgb.height=300;rgb.channels=3;
 std::array<uint8_t,2*PageBytes> colors{};
 for(unsigned i=0;i<colors.size();++i)colors[i]=uint8_t((i*193u+(i>>7)*71u)&255);
 rgb.pages[0]=colors.data();rgb.pages[1]=colors.data()+PageBytes;
 auto original=[&](int u,int v,unsigned channel,bool repeat){
  if(repeat){u=int(unsigned(u)&1048575u);v=int(unsigned(v)&1048575u);}else{u=bound(u,0,1048576);v=bound(v,0,1048576);}
  const int px=u*int(rgb.width)-524288,py=v*int(rgb.height)-524288;
  const int x=px>>20,y=py>>20;const unsigned fx=unsigned(px)&1048575u,fy=unsigned(py)&1048575u;
  auto value=[&](int x,int y){if(repeat){x=(x+int(rgb.width))%int(rgb.width);y=(y+int(rgb.height))%int(rgb.height);}else{x=bound(x,0,int(rgb.width)-1);y=bound(y,0,int(rgb.height)-1);}return unsigned(colors[(y*rgb.width+x)*3+channel]);};
  const unsigned a=value(x,y)*(1048576-fx)+value(x+1,y)*fx;
  const unsigned b=value(x,y+1)*(1048576-fx)+value(x+1,y+1)*fx;
  const unsigned q=unsigned((uint64_t(a)*(1048576-fy)+uint64_t(b)*fy+524288)>>20);
  return int((uint64_t(q)*16843009u+2147483648u)>>32);
 };
 for(unsigned i=0;i<30000;++i){random=random*1664525u+1013904223u;const int u=int(random&0xffffff)-8388608;random=random*1664525u+1013904223u;const int v=int(random&0xffffff)-8388608;const bool repeat=i&1;int r,g;rgb.sample_rg(u,v,repeat,r,g);CHECK(r==original(u,v,0,repeat));CHECK(g==original(u,v,1,repeat));CHECK(rgb.sample(u,v,2,repeat)==original(u,v,2,repeat));}
 // Shader clock wraps independently of the body and remains defined at the
 // full uint32 clock boundary. Synthetic noise avoids source texture copies.
 uint8_t byte=127;Field fields[5];for(auto&f:fields){f.width=f.height=1;f.channels=3;static const uint8_t rgb[]={127,127,127};f.pages[0]=rgb;}
 (void)byte;Model model;for(unsigned i=0;i<4;++i){model.q[i*5]=model.q[i*5+1]=model.q[i*5+4]=1<<24;model.q[i*5+2]=int(i+1)*12345;model.q[i*5+3]=-int(i+1)*67890;}
 model.q[20]=model.q[21]=model.q[24]=model.q[25]=1<<24;model.q[29]=model.q[31]=1<<24;
 for(uint32_t clock:{0u,1u,16667u,19999u,20000u,0xffffffffu}){Shader a,b;a.bind(fields,model,clock);b.bind(fields,model,clock%20000);for(int uv:{0,1,41943,524288,1048575}){auto x=a.sample(uv,1048576-uv),y=b.sample(uv,1048576-uv);CHECK(x.inner==y.inner&&x.alpha==y.alpha&&x.alpha<=1048576&&x.inner<=1048576);}}
 std::printf("Fire source table/sampling/clock: %u checks passed\n",checks);
}
