#include "../src/app.cpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
namespace spire {namespace spark_reference {
#include "fixtures/spark_texture_legacy.inc"
}}
using namespace spire;
using Bytes=std::vector<uint8_t>;
static App a;static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"spark texture:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void push_word(Bytes&out,unsigned n){out.push_back(uint8_t(n));out.push_back(uint8_t(n>>8));}
static void sparse(){
 // More than64 KiB of native spans proves that row offsets cannot be16-bit.
 Bytes bytes;std::vector<unsigned> red(128*128),alpha(128*128);
 for(unsigned y=0;y<128;++y){push_word(bytes,y%7?128:0);
  if(y%7)for(unsigned x=0;x<128;++x){
   const unsigned gray=(x*13+y*7)&31,coverage=(x*3+y*19)&255;
   push_word(bytes,x);push_word(bytes,1);bytes.push_back(1);push_word(bytes,gray<<11);bytes.push_back(uint8_t(coverage));
   red[y*128+x]=(gray<<3)|(gray>>2);alpha[y*128+x]=coverage;
  }
 }
 CHECK(bytes.size()>65535);SparkTexture texture;CHECK(texture.bind({bytes.data(),nullptr,nullptr,128,128,4}));
 for(unsigned v=0;v<1024;++v)for(unsigned u=0;u<1024;++u){
  unsigned r=0,coverage=0;texture.sample(u,v,r,coverage);const unsigned at=(v/8)*128+u/8;
  CHECK(r==red[at]&&coverage==alpha[at]);
 }
 CHECK(sizeof(SparkTexture)<=576);
}
static Bytes read(const std::string&path){
 FILE*f=std::fopen(path.c_str(),"rb");CHECK(f&&!std::fseek(f,0,SEEK_END));const long length=std::ftell(f);CHECK(length>40&&!std::fseek(f,0,SEEK_SET));
 Bytes b(static_cast<size_t>(length));CHECK(std::fread(b.data(),1,b.size(),f)==b.size()&&!std::fclose(f));return b;
}
static void actual(const char*folder){
 const std::string root=folder;auto common=read(root+"/nSpire2.dat.tns"),locale=read(root+"/nSpire2.eng.dat.tns");SplitResourceSource split;
 CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(locale.data(),locale.size()),ResourceLanguage::English));
 CHECK(a.init(split.source(),{}));
 for(unsigned id:{I7_GlowSpeck,I7_Speck,I7_Glow,I7_BlockRing,I7_HitStreak}){
  SparkTexture texture;auto reference=std::make_unique<spark_reference::SparkTexture>();
  CHECK(texture.read(a.content,id)&&reference->read(a.content,id));
  for(unsigned v=0;v<1024;++v)for(unsigned u=0;u<1024;++u){
   unsigned r=0,coverage=0,rr=0,ra=0;texture.sample(u,v,r,coverage);reference->sample(u,v,rr,ra);CHECK(r==rr&&coverage==ra);
  }
 }
 std::vector<uint16_t> reference(320*240);
 const auto reset=[&](bool blocked){
  for(unsigned i=0;i<320*240;++i)a.pixels[i]=uint16_t(i*77u);
  a.combat_sparks.reset();a.combat_sparks.emit(1000,157,131,blocked,0x7ea041);
  a.combat_sparks.emit(1020,245,119,!blocked,0x9821bb);
 };
 for(bool blocked:{false,true})for(unsigned age:{0u,33u,111u,199u,299u,449u,749u}){
  reset(blocked);Canvas c{a};spark_reference::combat_hit_sparks(c,1000+age);
  std::memcpy(reference.data(),a.pixels,reference.size()*sizeof(uint16_t));
  reset(blocked);combat_hit_sparks(c,1000+age);
  for(unsigned i=0;i<320*240;++i)CHECK(reference[i]==a.pixels[i]);CHECK(a.content.healthy());
 }
}
int main(int argc,char**argv){CHECK(argc==1||argc==2);sparse();if(argc==2)actual(argv[1]);
 std::printf("spark texture: %u checks; original sparse/dense samples and five-system whole-frame pixels unchanged; runtime sampler %zu bytes\n",checks,sizeof(SparkTexture));}
