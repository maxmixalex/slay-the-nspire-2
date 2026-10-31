// Full source-reference frames go through the production registered Canvas
// and range reader. Source references are private RGB565/alpha8 spools.
#include "../src/app.cpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <vector>
#include <algorithm>
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
static int reference_floor(int64_t value,int divisor){
 const auto quotient=value/divisor,remainder=value%divisor;
 return int(quotient-(remainder<0));
}
static void reference_picture(Canvas&c,Picture p,int anchor_x,int anchor_y,int source_x,int source_y,
                              int scale,unsigned mode,uint16_t(*transform)(uint16_t)=nullptr){
 // Independent full-source inverse sampling. The reference uses the entire
 // original raster and knows nothing about piece extents or the span walker.
 int columns[320],rows[240];
 for(int x=0;x<320;++x)columns[x]=reference_floor(int64_t(x-anchor_x)*1024+512,scale)-source_x;
 for(int y=0;y<240;++y)rows[y]=reference_floor(int64_t(y-anchor_y)*1024+512,scale)-source_y;
 for(int y=0;y<240;++y){if(rows[y]<0||rows[y]>=p.h)continue;
  for(int x=0;x<320;++x){if(columns[x]<0||columns[x]>=p.w)continue;
   const unsigned index=unsigned(rows[y])*p.w+unsigned(columns[x]),alpha=p.alpha[index];if(!alpha)continue;
   const uint8_t*pixel=p.pixels+index*2;uint16_t color=uint16_t(pixel[0]|unsigned(pixel[1])<<8);
   if(transform)color=transform(color);c.paint(y*320+x,color,alpha,mode);
  }
 }
}
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
 unsigned views=0,visible_parts=0,scaled_views=0,gray_views=0;uint64_t exact_pixels=0;static uint16_t expected[320*240];static State before;before=a.game;
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
  std::vector<int> projections{scale};
  const int normal=int(uint64_t(old_units)*4/21);
  for(const int projection:{normal,normal/2,int(uint64_t(old_units)*218/1080),
      int(uint64_t(old_units)*760006*218/1080000000),normal*1177/1024,normal*1536/1024,normal*2048/1024})
   if(projection>0&&projection<=scale&&std::find(projections.begin(),projections.end(),projection)==projections.end())projections.push_back(projection);
  const int character=clip>=int(CharacterDeathAnimationFirst)&&clip<int(MerchantDeathAnimationFirst+5)?
      (clip-int(CharacterDeathAnimationFirst))%5:clip==int(DefectDeathParticles)?2:clip==int(RegentDeathParticles)?3:-1;
  if(character>=0){
   const auto registration=player_sprite_registration(Character(character));
   // Repeat game_over_actor's integer operation order, including its
   // registration conversion after the room projection. A single combined
   // fraction can differ by one target scale unit.
   const int ordinary=int(registration.units)*4/21;
   for(const int room_scale:{ordinary,ordinary*1536/1024,int(registration.units)*218/1080,
       int(int64_t(registration.units)*760006*218/1080000000),architect_scene_pose(Character(character),false).scale}){
    const int projection=int(int64_t(room_scale)*old_units/registration.units);
    if(projection>0&&projection<=scale&&std::find(projections.begin(),projections.end(),projection)==projections.end())projections.push_back(projection);
   }
  }
  for(const int projection:projections){const int effective=int(uint64_t(projection)*units/old_units);CHECK(effective>0&&effective<=1024);
   for(const auto anchor:anchors){
    background();reference_picture(c,source,anchor.first,anchor.second,left,top,effective,mode);
    std::memcpy(expected,a.pixels,sizeof expected);background();
    CHECK(c.framed_clip(clip,anchor.first,anchor.second,0,projection,false,mode,int(frame)));
    if(std::memcmp(expected,a.pixels,sizeof expected)){
     for(unsigned p=0;p<320*240;++p)if(expected[p]!=a.pixels[p]){std::fprintf(stderr,"source mismatch clip %d frame %u projection %d/%d at %u,%u expected %04x actual %04x\n",clip,frame,projection,effective,p%320,p/320,expected[p],a.pixels[p]);break;}
     CHECK(false);
    }
    exact_pixels+=320*240;++views;scaled_views+=effective!=1024;
    Canvas::FramedDraw shown[FramedUi20MaxLayers];unsigned count=0;
    CHECK(c.framed_collect(clip,anchor.first,anchor.second,frame*ms,projection,false,shown,count));visible_parts+=count;
    if(mode==0){background();c.scaled_clip(clip,anchor.first,anchor.second,frame*ms,projection,false);CHECK(!std::memcmp(expected,a.pixels,sizeof expected));}
    if(frame==seen[index].size()/2){
     Canvas faded=c;faded.opacity=173;background();reference_picture(faded,source,anchor.first,anchor.second,left,top,effective,mode);
     std::memcpy(expected,a.pixels,sizeof expected);background();CHECK(faded.framed_clip(clip,anchor.first,anchor.second,0,projection,false,mode,int(frame)));
     CHECK(!std::memcmp(expected,a.pixels,sizeof expected));exact_pixels+=320*240;
     if(mode==0){background();reference_picture(c,source,anchor.first,anchor.second,left,top,effective,0,doom_grayscale);
      std::memcpy(expected,a.pixels,sizeof expected);background();
      for(unsigned piece=0;piece<count;++piece)c.framed_picture(shown[piece],255,0,doom_grayscale);
      CHECK(!std::memcmp(expected,a.pixels,sizeof expected));exact_pixels+=320*240;++gray_views;
     }
    }
   }
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
 std::printf("actor UI22: %llu checks, %u clips, %u original-clock frames, %u views (%u non-native, %u gray), %u visible pieces, %llu exact source-reference LCD pixels; invisible pieces require no source reads\n",(unsigned long long)checks,clips,cases,views,scaled_views,gray_views,visible_parts,(unsigned long long)exact_pixels);
}
