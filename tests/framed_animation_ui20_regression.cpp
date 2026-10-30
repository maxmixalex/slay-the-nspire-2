// Actual private source frames exercise the production draw path and bounded
// decoder. Synthetic registration/pixel equivalence is tested separately.
#include "../src/app.cpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <vector>
using namespace spire;
static App a;static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"framed UI20:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static uint32_t pixels(){uint32_t result=2166136261u;for(const auto p:a.pixels)result=(result^p)*16777619u;return result;}
static void capture(const char*directory,const char*name){
 if(!directory)return;std::filesystem::create_directories(directory);const auto path=std::filesystem::path(directory)/(std::string(name)+".ppm");FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");
 for(const auto p:a.pixels){const unsigned char c[]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};CHECK(std::fwrite(c,1,3,f)==3);}CHECK(!std::fclose(f));
}
int main(int argc,char**argv){
 CHECK(argc==2||argc==3);FILE*f=std::fopen(argv[1],"rb");CHECK(f);std::vector<uint8_t>data(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size()&&!std::fclose(f));CHECK(a.init(data.data(),data.size(),{}));
 CHECK(a.content.ui_image(FramedUi20AuxiliaryCount-1)>=0);Canvas c{a};const char*directory=argc==3?argv[2]:nullptr;
 const int clips[]={105,123,124,125,548,706,707,1263,1626,FramedUi20OstyDeadLoop};
 unsigned frames=0,parts=0;static State before;before=a.game;
 for(const int clip:clips){
  const auto parameters=a.content.image(a.content.ui_image(FramedUi20BaseAuxiliary+unsigned(framed_ui20_row(clip))));CHECK(parameters.pixels);
  const unsigned count=Canvas::vantom_word(parameters.pixels,2),ms=Canvas::vantom_word(parameters.pixels,3);
  const unsigned reference=Canvas::vantom_word(parameters.pixels,6)|(Canvas::vantom_word(parameters.pixels,7)<<16);
  const unsigned units=Canvas::vantom_word(parameters.pixels,4)|(Canvas::vantom_word(parameters.pixels,5)<<16);
  const int scale=int((uint64_t(1024)*reference+units-1)/units);CHECK(count>0&&count<=256&&ms);
  uint32_t first=0,last=0;bool varying=false;
  for(unsigned frame=0;frame<count;++frame){
   Canvas::FramedDraw full[FramedUi20MaxLayers],visible[FramedUi20MaxLayers];unsigned all=0,shown=0;
   CHECK(c.framed_collect(clip,160,190,frame*ms,scale,false,full,all,false));CHECK(all>0&&all<=FramedUi20MaxLayers);
   CHECK(c.framed_collect(clip,160,190,frame*ms,scale,false,visible,shown));CHECK(shown<=all);
   for(unsigned i=0;i<all;++i){const auto image=a.content.image(full[i].image);CHECK(image.pixels&&image.w<=320&&image.h<=240);++parts;}
   for(unsigned i=0;i<shown;++i)CHECK(visible[i].x<320&&visible[i].y<240&&visible[i].x+visible[i].w>0&&visible[i].y+visible[i].h>0);
   std::memset(a.pixels,0,sizeof a.pixels);c.scaled_clip(clip,160,190,frame*ms,scale,false);const auto value=pixels();
   if(!frame)first=value;else varying|=first!=value;last=value;++frames;
   if(directory&&frame==count/3){char name[64];std::snprintf(name,sizeof name,"clip-%d-native-frame",clip);capture(directory,name);}
  }
  // The original short Osty dead_loop samples retain one corpse pose at
  // LCD precision. A loop is not required to invent visible movement.
  CHECK(varying||clip==FramedUi20OstyDeadLoop);std::memset(a.pixels,0,sizeof a.pixels);c.scaled_clip(clip,160,190,count*ms,scale,true);CHECK(pixels()==first);
  std::memset(a.pixels,0,sizeof a.pixels);c.scaled_clip(clip,160,190,count*ms,scale,false);CHECK(pixels()==last);
  Canvas::FramedDraw invisible[FramedUi20MaxLayers];unsigned n=0;CHECK(c.framed_collect(clip,10000,10000,0,scale,true,invisible,n)&&!n);
 }
 uint32_t icon_pixels[4]{};const unsigned icons[]={U_RewardCard,I20_RewardRare,I20_RewardUncommon,I20_RewardSpecial};
 for(unsigned i=0;i<4;++i){std::memset(a.pixels,0,sizeof a.pixels);const int image=a.content.ui_image(icons[i]);CHECK(image>=0);c.picture(image,0,0,23,23);icon_pixels[i]=pixels();for(unsigned j=0;j<i;++j)CHECK(icon_pixels[i]!=icon_pixels[j]);}
 CHECK(!std::memcmp(&before,&a.game,sizeof before)&&a.content.healthy());
 std::printf("framed UI20: %u checks, %u original-clock frames, %u bounded parts, exact loop/clamp and distinct source reward icons\n",checks,frames,parts);
}
