// Full source-reference frames go through the production registered Canvas
// and range reader. Source references are private RGB565/alpha8 spools.
#include "../src/app.cpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <vector>
using namespace spire;
static App a;static uint64_t checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"actor UI22:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static unsigned half(const uint8_t*p){return p[0]|unsigned(p[1])<<8;}
static uint32_t word(const uint8_t*p){return half(p)|uint32_t(half(p+2))<<16;}
static bool read_at(void*context,uint32_t offset,void*out,uint32_t count){
 const auto&b=*(const std::vector<uint8_t>*)context;
 if(offset>b.size()||count>b.size()-offset)return false;
 std::memcpy(out,b.data()+offset,count);return true;
}
static uint32_t read_word(FILE*f){uint8_t b[4];CHECK(std::fread(b,1,4,f)==4);return word(b);}
static void background(){for(unsigned y=0;y<240;++y)for(unsigned x=0;x<320;++x)a.pixels[y*320+x]=uint16_t(0x4a69u^(x*13u+y*29u));}
static void capture(const char*directory,int clip,unsigned frame){
 if(!directory)return;std::filesystem::create_directories(directory);
 const auto path=std::filesystem::path(directory)/("clip-"+std::to_string(clip)+"-frame-"+std::to_string(frame)+".ppm");
 FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");
 for(auto p:a.pixels){const uint8_t c[]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};CHECK(std::fwrite(c,1,3,f)==3);}CHECK(!std::fclose(f));
}
int main(int argc,char**argv){
 CHECK(argc==3||argc==4);FILE*pack=std::fopen(argv[1],"rb");CHECK(pack);
 std::vector<uint8_t>data(spire_test::pack_size(pack));CHECK(std::fread(data.data(),1,data.size(),pack)==data.size()&&!std::fclose(pack));
 CHECK(a.init({&data,uint32_t(data.size()),read_at,nullptr},{}));
 CHECK(a.content.ui_image(FramedUi22AuxiliaryCount-1)>=0);Canvas c{a};
 std::vector<std::vector<bool>> seen(FramedUi22ClipCount+1);unsigned expected_frames=0,clips=0;
 for(unsigned index=0;index<=FramedUi22ClipCount;++index){
  const int clip=index==FramedUi22ClipCount?FramedUi20OstyDeadLoop:int(index);
  const int id=a.content.ui_image(FramedUi22BaseAuxiliary+index);
  if(index>=VantomUi15FirstClip&&index<VantomUi15FirstClip+VantomUi15ClipCount)CHECK(id==0);
  if(id<=0)continue;
  const auto table=a.content.image(id);CHECK(table.pixels&&table.format==0);
  const unsigned count=Canvas::vantom_word(table.pixels,2);CHECK(count&&count<=256);
  if(clip>=0)CHECK(a.content.clip(clip).count==count);
  seen[index].resize(count);expected_frames+=count;++clips;
 }
 CHECK(clips&&expected_frames);FILE*reference=std::fopen(argv[2],"rb");CHECK(reference);
 char magic[8];CHECK(std::fread(magic,1,8,reference)==8&&!std::memcmp(magic,"ACTR0022",8));
 const unsigned cases=read_word(reference);CHECK(cases==expected_frames);
 unsigned views=0,visible_parts=0;uint64_t exact_pixels=0;static uint16_t expected[320*240];static State before;before=a.game;
 for(unsigned n=0;n<cases;++n){
  const int clip=int32_t(read_word(reference));const unsigned frame=read_word(reference),ms=read_word(reference);
  const int scale=int32_t(read_word(reference)),left=int32_t(read_word(reference)),top=int32_t(read_word(reference));
  const unsigned width=read_word(reference),height=read_word(reference),mode=read_word(reference),view_count=read_word(reference),size=read_word(reference),reserved=read_word(reference);
  CHECK(clip==FramedUi20OstyDeadLoop||(clip>=0&&unsigned(clip)<FramedUi22ClipCount));
  const unsigned index=clip==FramedUi20OstyDeadLoop?FramedUi22ClipCount:unsigned(clip);
  CHECK(frame<seen[index].size()&&!seen[index][frame]);seen[index][frame]=true;
  CHECK(width&&width<=2560&&height&&height<=1920&&size==width*height*3&&mode<=1&&!reserved&&view_count&&view_count<=FramedUi20MaxLayers);
  std::vector<std::pair<int,int>> anchors;
  for(unsigned i=0;i<view_count;++i){const int x=int32_t(read_word(reference)),y=int32_t(read_word(reference));anchors.emplace_back(x,y);}
  std::vector<uint8_t>raw(size);CHECK(std::fread(raw.data(),1,size,reference)==size);
  const Picture source{raw.data(),raw.data()+width*height*2,nullptr,uint16_t(width),uint16_t(height),1};
  const int parameter=c.framed_parameters(clip);CHECK(parameter>0);
  const auto table=a.content.image(parameter);CHECK(table.pixels&&Canvas::vantom_word(table.pixels,3)==ms);
  const unsigned units=Canvas::vantom_word(table.pixels,4)|(Canvas::vantom_word(table.pixels,5)<<16);
  const unsigned old_units=Canvas::vantom_word(table.pixels,6)|(Canvas::vantom_word(table.pixels,7)<<16);
  CHECK(uint64_t(scale)*units/old_units==1024); // Native-reference density; never enlarged source samples.
  for(const auto anchor:anchors){
   background();c.picture(source,anchor.first+left,anchor.second+top,int(width),int(height),255,mode);
   std::memcpy(expected,a.pixels,sizeof expected);background();
   CHECK(c.framed_clip(clip,anchor.first,anchor.second,0,scale,false,mode,int(frame)));
   if(std::memcmp(expected,a.pixels,sizeof expected)){
    for(unsigned p=0;p<320*240;++p)if(expected[p]!=a.pixels[p]){std::fprintf(stderr,"source mismatch clip %d frame %u at %u,%u expected %04x actual %04x\n",clip,frame,p%320,p/320,expected[p],a.pixels[p]);break;}
    CHECK(false);
   }
   exact_pixels+=320*240;++views;
   Canvas::FramedDraw shown[FramedUi20MaxLayers];unsigned count=0;
   CHECK(c.framed_collect(clip,anchor.first,anchor.second,frame*ms,scale,false,shown,count));visible_parts+=count;
   if(mode==0){background();c.scaled_clip(clip,anchor.first,anchor.second,frame*ms,scale,false);CHECK(!std::memcmp(expected,a.pixels,sizeof expected));}
  }
  // An invisible actor may consult its resident parameter, but may not read
  // or decode any screen-excluded piece. Cache the parameter before counting.
  CHECK(a.content.image(parameter).pixels);const auto initial=a.content.read_stats();
  Canvas::FramedDraw invisible[FramedUi20MaxLayers];unsigned count=0;
  CHECK(c.framed_collect(clip,100000,100000,0,scale,false,invisible,count,true,int(frame))&&!count);
  CHECK(c.framed_clip(clip,100000,100000,0,scale,false,mode,int(frame)));
  const auto after=a.content.read_stats();CHECK(initial.callbacks==after.callbacks&&initial.callback_bytes==after.callback_bytes);
  if(argc==4&&(clip==1762||clip==1757||clip==35||clip==FramedUi20OstyDeadLoop)&&frame==seen[index].size()/2)capture(argv[3],clip,frame);
  CHECK(a.content.healthy());
  if((n+1)%1000==0){std::printf("actor source Canvas: %u/%u frames, %u views, %llu exact pixels\n",n+1,cases,views,(unsigned long long)exact_pixels);std::fflush(stdout);}
 }
 CHECK(std::fgetc(reference)==EOF&&!std::ferror(reference)&&!std::fclose(reference));
 CHECK(!std::memcmp(&before,&a.game,sizeof before));
 std::printf("actor UI22: %llu checks, %u clips, %u original-clock frames, %u views, %u visible pieces, %llu exact source-reference LCD pixels; invisible pieces require no source reads\n",(unsigned long long)checks,clips,cases,views,visible_parts,(unsigned long long)exact_pixels);
}
