// Exercise the private complete shop renderer with its original, unculled
// branch as the whole-frame oracle. No resource pixels are embedded here.
#include "../src/app.cpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
namespace {
static App app;
static SaveData before,after;
static unsigned checks,frames,skipped;
static uint16_t expected[ScreenW*ScreenH];
static HitRegion expected_hits[MaxHits];
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"merchant occlusion:%d frame=%u clock=%u character=%u %s\n",__LINE__,frames,app.map_clock_ms,unsigned(app.game.character),#x);std::exit(1);}}while(0)
void surface(){for(unsigned i=0;i<ScreenW*ScreenH;++i)app.pixels[i]=uint16_t((i*3917u)^(i>>2)^0x8410);app.hit_count=0;}
void unchanged(){CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
void setup(Character character){
 new_run(app.game,73,false,character);app.game.map[0].room=Room::Shop;CHECK(choose_node(app.game,0));
 app.title=app.library=app.paused=app.help=app.character_select=app.shop_open=false;
 app.map_clock_ms=app.animation=0;app.reset_presentation();app.pointer.visible=false;app.cursor=1;
 CHECK(save(app.game,before));
}
void compare(uint32_t clock){
 ++frames;app.map_clock_ms=clock;app.animation=clock/33;
 const auto presentation=app.room_presentation;const int cursor=app.cursor;
 surface();Canvas reference{app};shop_screen(reference,false);CHECK(app.content.healthy());
 std::memcpy(expected,app.pixels,sizeof expected);const int count=app.hit_count;
 std::copy(app.hits,app.hits+count,expected_hits);const auto final=app.room_presentation.merchant;
 app.room_presentation=presentation;app.cursor=cursor;surface();Canvas candidate{app};shop_screen(candidate,true);
 CHECK(app.content.healthy());
 if(std::memcmp(expected,app.pixels,sizeof expected)){
  for(unsigned i=0;i<ScreenW*ScreenH;++i)if(expected[i]!=app.pixels[i]){
   std::fprintf(stderr,"pixel (%u,%u): %04x expected %04x\n",i%ScreenW,i/ScreenW,app.pixels[i],expected[i]);break;
  }CHECK(false);
 }
 CHECK(app.hit_count==count);
 for(int i=0;i<count;++i){const auto&a=app.hits[i];const auto&b=expected_hits[i];
  CHECK(a.x==b.x&&a.y==b.y&&a.w==b.w&&a.h==b.h&&a.kind==b.kind&&a.value==b.value&&a.enabled==b.enabled);
 }
 CHECK(!std::memcmp(&final,&app.room_presentation.merchant,sizeof final));
}
void prove_hidden(const std::vector<uint8_t>&pack,Character character){
 CHECK(app.content.open(pack.data(),pack.size()));setup(character);app.shop_open=true;
 app.room_presentation.merchant.set_open(true,0);app.map_clock_ms=2000;app.animation=2000/33;
 const auto clip=app.content.clip(AnimMerchantIronclad+int(character));CHECK(clip.count);
 const int image=int(clip.first+2000/clip.frame_ms%clip.count);const auto size=app.content.image_dimensions(image);
 Canvas c{app};const auto rug=app.content.image(app.content.ui_image(U_MerchantRug));
 const bool hidden=c.opaque_picture_covers(rug,MerchantInventoryLayout::RugX,MerchantInventoryLayout::RugY,
  MerchantInventoryLayout::RugW,MerchantInventoryLayout::RugH,-clip.pivot_x,22-clip.pivot_y,size.w,size.h);
 CHECK(!app.content.image_cached(image));surface();shop_screen(c,true);
 if(hidden){++skipped;CHECK(!app.content.image_cached(image));}else CHECK(app.content.image_cached(image));
 surface();shop_screen(c,false);CHECK(app.content.image_cached(image));unchanged();
}
}
int main(int argc,char**argv){
 if(argc!=2){std::fprintf(stderr,"Usage: merchant_occlusion_regression OWNER_PACK\n");return 2;}
 FILE*f=std::fopen(argv[1],"rb");CHECK(f&&std::fseek(f,0,SEEK_END)==0);const long length=std::ftell(f);
 CHECK(length>=40&&uint64_t(length)<=MaxContentFile&&std::fseek(f,0,SEEK_SET)==0);std::vector<uint8_t>pack(static_cast<size_t>(length));
 CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size()&&!std::fclose(f));
 CHECK(app.content.open(pack.data(),pack.size())&&app.content.current_profile());
 for(unsigned i=0;i<CharacterKinds;++i){
  setup(Character(i));compare(0);app.shop_open=true;
  // Every opening/closing phase, including a close interrupted by reopening,
  // takes the full original draw path and still compares the entire surface.
  for(unsigned t=0;t<=1100;t+=20)compare(t);
  const auto clip=app.content.clip(AnimMerchantIronclad+int(i));
  const unsigned duration=std::max(6000u,unsigned(clip.count)*clip.frame_ms);
  for(unsigned t=0;t<=duration;t+=33)compare(1200+t);
  uint32_t now=1200+duration;compare(now);app.shop_open=false;
  for(unsigned t=0;t<=900;t+=20)compare(now+t);
  now+=1000;app.shop_open=true;compare(now);compare(now+260);app.shop_open=false;compare(now+280);compare(now+400);
  app.shop_open=true;for(unsigned t=0;t<=1100;t+=20)compare(now+420+t);
  for(uint32_t t:{5999u,6000u,12031u,12032u,0xfffffffeu,0xffffffffu})compare(t);
  unchanged();prove_hidden(pack,Character(i));
 }
 CHECK(skipped>0);
 std::printf("Merchant occlusion: %u checks; %u exact whole frames, five full character cycles, opening/closing/interruption, clock boundaries, matching controls/state, %u hidden character frames avoid decode\n",checks,frames,skipped);
}
