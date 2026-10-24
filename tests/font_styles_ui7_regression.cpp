// Exercise the actual private Canvas without adding a production test API.
#include "../src/app.cpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
namespace {
using Bytes=std::vector<uint8_t>;
static App app;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"font styles:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void put(Bytes&b,unsigned at,uint32_t v,unsigned n=4){CHECK(at+n<=b.size());for(unsigned i=0;i<n;++i)b[at+i]=uint8_t(v>>(i*8));}
static Bytes fixture(bool modern=true){
 const unsigned ni=A_Count,ng=modern?6:3,ns=modern?Ui7StringCount:LocaleStringCount;
 const unsigned nc=modern?Ui7AnimationKinds:MerchantEffectsAnimationKinds,nu=modern?unsigned(Ui7AuxiliaryCount):unsigned(Ui6AuxiliaryCount);
 const unsigned ga=40+ni*16,sa=ga+ng*16,ca=sa+ns*8,base=ca+nc*16+CardKinds*16+nu*4;
 const unsigned table=base+64,costs=table+16,descriptors=costs+((ni+1)&~1u);
 Bytes b(descriptors+16);put(b,0,0x3250534e);put(b,4,modern?Ui7ContentVersion:Ui6ContentVersion);put(b,8,0x41cef1ea);
 put(b,16,ni);put(b,20,ng);put(b,24,ns);put(b,28,nc);put(b,32,CardKinds);
 const unsigned fields[]={0x314c4954,1,16,14,1,1,1,1,descriptors,table,770,1,524288,617323,costs,ni};
 for(unsigned i=0;i<16;++i)put(b,base+i*4,fields[i]);
 const unsigned image=unsigned(b.size());b.push_back(0);b.push_back(0);
 for(unsigned i=0;i<ni;++i){put(b,40+i*16,image);put(b,44+i*16,1,2);put(b,46+i*16,1,2);put(b,52+i*16,2);}
 const unsigned cp[]={63,65,66,65,65,65},size[]={8,8,8,72,136,200},advance[]={1,2,3,5,4,6};
 const Bytes alpha[]={{127},{255},{127},{255,0,255},{0,255},{0,255,255,0}};
 for(unsigned i=0;i<ng;++i){const unsigned at=ga+i*16;put(b,at,cp[i]);put(b,at+4,unsigned(b.size()));put(b,at+8,size[i],2);
  b[at+10]=uint8_t(alpha[i].size());b[at+11]=1;put(b,at+12,advance[i],2);b.insert(b.end(),alpha[i].begin(),alpha[i].end());}
 const unsigned empty=unsigned(b.size());b.push_back(0);const unsigned marker=unsigned(b.size());b.insert(b.end(),{'e','n','g',0});
 for(unsigned i=0;i<ns;++i)put(b,sa+i*8,empty);put(b,sa+LocaleMarkerString*8,marker);put(b,sa+LocaleMarkerString*8+4,3);
 for(unsigned i=0;i<nc;++i){put(b,ca+i*16+4,1,2);put(b,ca+i*16+6,33,2);}put(b,12,unsigned(b.size()));return b;
}
static void clear(){std::fill(std::begin(app.pixels),std::end(app.pixels),0);}
static uint16_t pixel(unsigned x,unsigned y){return app.pixels[y*320+x];}
static void direct(){
 auto bytes=fixture();CHECK(app.content.open(bytes.data(),bytes.size()));Canvas c{app};
 CHECK(app.content.glyph_metrics('A',8).advance==2);
 CHECK(app.content.glyph_metrics('A',8,1).advance==5);
 CHECK(app.content.glyph_metrics('A',8,2).advance==4);
 CHECK(app.content.glyph_metrics('A',8,3).advance==6);
 CHECK(app.content.glyph_metrics('B',8,2).advance==3); // unbaked style uses same regular codepoint
 CHECK(app.content.glyph_metrics('Z',8,1).advance==1); // then ordinary ? fallback
 CHECK(c.width("A[b]A[/b][i]A[/i][b][i]A[/i][/b]",8)==17);
 CHECK(c.width("[b]A[/b]B",8)==8);
 clear();CHECK(c.text("[b]A[/b]B",10,10,8,0xffff)==18);
 CHECK(pixel(10,10)==0xffff&&pixel(12,10)==0xffff);
 CHECK(pixel(11,10)==0&&pixel(13,10)==0); // no displaced duplicate to fake bold
 CHECK(pixel(15,10)!=0&&pixel(14,10)==0&&pixel(16,10)==0);
 clear();CHECK(c.paragraph("[b]A[/b]B",10,20,6,3,0,8,0xffff)==2);
 CHECK(pixel(10,20)==0xffff&&pixel(12,20)==0xffff&&pixel(10,31)!=0);
 clear();c.centered("[b]A[/b]B",10,40,20,8,0xffff);
 CHECK(pixel(16,40)==0xffff&&pixel(18,40)==0xffff&&pixel(21,40)!=0);
 clear();auto plan=c.fitted("[b]A[/b]B",10,50,8,10,8,8,0xffff,false,true);
 CHECK(!plan.truncated&&plan.rows==1);CHECK(pixel(10,50)==0xffff&&pixel(15,50)!=0);
 auto legacy=fixture(false);CHECK(app.content.open(legacy.data(),legacy.size()));
 CHECK(app.content.glyph_metrics('A',8,1).advance==2);CHECK(c.width("[b]A[/b]B",8)==5);
}
struct Provider {
 const Bytes*bytes;unsigned calls=0;
 static bool read(void*context,uint32_t off,void*out,uint32_t count){auto&p=*static_cast<Provider*>(context);++p.calls;
  if(off>p.bytes->size()||count>p.bytes->size()-off)return false;std::memcpy(out,p.bytes->data()+off,count);return true;}
 ResourceSource source(){return {this,uint32_t(bytes->size()),read,nullptr};}
};
static void provider(){
 auto bytes=fixture();Provider p{&bytes};CHECK(app.content.open(p.source()));const auto opened=p.calls;
 for(unsigned face=0;face<4;++face)CHECK(app.content.glyph_metrics('A',8,face).advance>0);
 CHECK(p.calls==opened);for(unsigned face=0;face<4;++face)CHECK(app.content.glyph('A',8,face).pixels);
 const auto warm=p.calls;CHECK(warm>opened);
 for(unsigned repeat=0;repeat<20;++repeat)for(unsigned face=0;face<4;++face)CHECK(app.content.glyph('A',8,face).pixels);
 CHECK(p.calls==warm&&app.content.healthy());
}
}
int main(){direct();provider();std::printf("font styles: %u checks\n",checks);return 0;}
