#include "app.hpp"
#include "combat_layout.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App app;
static SaveData saved,after;
static unsigned checks;
static const char* screenshots;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"merchant line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
using Bytes=std::vector<uint8_t>;
static Bytes read(const char*path){
 FILE*f=std::fopen(path,"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long n=std::ftell(f);
 CHECK(n>0&&n<=MaxContentFile);CHECK(!std::fseek(f,0,SEEK_SET));Bytes bytes(n);
 CHECK(std::fread(bytes.data(),1,bytes.size(),f)==bytes.size());CHECK(!std::fclose(f));return bytes;
}
static void put(Bytes&b,unsigned at,uint32_t value){for(unsigned i=0;i<4;++i)b[at+i]=uint8_t(value>>(8*i));}
static void capture(const char*name){
 if(!screenshots)return;const std::string path=std::string(screenshots)+"/"+name+".ppm";
 FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");
 uint8_t row[960];for(unsigned y=0;y<240;++y){for(unsigned x=0;x<320;++x){const unsigned p=app.pixels[y*320+x];
  row[x*3]=uint8_t((p>>11)*255/31);row[x*3+1]=uint8_t(((p>>5)&63)*255/63);row[x*3+2]=uint8_t((p&31)*255/31);}
  CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static void unchanged(){CHECK(save(app.game,after));CHECK(saved.size==after.size&&!std::memcmp(saved.bytes,after.bytes,saved.size));}
static unsigned differences(const std::vector<uint16_t>&other,int x0,int y0,int x1,int y1){
 unsigned n=0;for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x)n+=app.pixels[y*320+x]!=other[y*320+x];return n;
}
static std::vector<uint16_t> pixels(){return {app.pixels,app.pixels+320*240};}
static const HitRegion* action(UiAction id){for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::UiAction&&app.hits[i].value==int(id))return &app.hits[i];return nullptr;}
static void shop(Character character){
 new_run(app.game,73,false,character);app.game.map[0].room=Room::Shop;CHECK(choose_node(app.game,0));
 app.title=app.library=app.paused=app.help=app.character_select=false;app.reset_presentation();
 app.pointer.visible=false;app.map_clock_ms=1375;app.cursor=1;CHECK(save(app.game,saved));app.render();
 CHECK(app.game.phase==Phase::Shop&&!app.shop_open);const auto*body=action(UiAction::OpenShop);CHECK(body);
 CHECK(body->x==MerchantLayout::BodyX-2&&body->y==MerchantLayout::BodyY-2&&body->w==MerchantLayout::BodyW+4&&body->h==MerchantLayout::BodyH+4);
 CHECK(action(UiAction::LeaveShop));unsigned openings=0;for(int i=0;i<app.hit_count;++i)openings+=app.hits[i].kind==HitKind::UiAction&&app.hits[i].value==int(UiAction::OpenShop);CHECK(openings==1);
}
static void phase_and_focus(){
 const Clip base=app.content.clip(AnimMerchantRoomTop),focused=app.content.clip(AnimMerchantRoomFocused);
 CHECK(base.count==145&&focused.count==base.count&&focused.frame_ms==base.frame_ms&&focused.frame_ms==41);
 for(int id:{AnimMerchantFlamesBehind,AnimMerchantFlamesFront}){const auto clip=app.content.clip(id);CHECK(clip.count==150&&clip.frame_ms==40);}
 for(unsigned ch=0;ch<5;++ch){
  shop(Character(ch));const auto unfocused=pixels();const uint32_t clock=app.map_clock_ms;
  CHECK(app.content.image_cached(base.first+(1375*base.count)/6000));
  CHECK(!app.content.image_cached(focused.first+(1375*focused.count)/6000)||ch>0);
  app.input(Key::Left);CHECK(app.cursor==0&&!app.pointer.visible);app.render();
  CHECK(app.map_clock_ms==clock&&app.content.image_cached(focused.first+(1375*focused.count)/6000));
  CHECK(differences(unfocused,202,108,272,190)>30);const auto keyboard=pixels();
  if(!ch)capture("merchant-outline-keyboard");
  // Pointer movement off the merchant clears a stale keyboard selection.
  app.pointer_move(160,35);app.render();CHECK(!differences(unfocused,202,108,272,190));
  CHECK(app.map_clock_ms==clock);if(!ch)capture("merchant-unfocused");
  // Pointer focus uses the same original skin and sample. The cursor itself
  // is deliberately below this comparison region.
  app.pointer_move(MerchantLayout::BodyX+MerchantLayout::BodyW/2,MerchantLayout::BodyY+MerchantLayout::BodyH-1);
  app.render();CHECK(!differences(keyboard,202,108,272,MerchantLayout::BodyY+MerchantLayout::BodyH-2));
  CHECK(app.map_clock_ms==clock);unchanged();
  app.pointer_button(true);app.pointer_button(false);CHECK(app.shop_open);app.render();unchanged();
  app.input(Key::Back);CHECK(!app.shop_open);app.render();unchanged();
  app.pointer_move(166,190);app.pointer_button(true);app.pointer_button(false);CHECK(!app.shop_open);unchanged();
 }
 shop(Character::Ironclad);app.cursor=0;app.map_clock_ms=0;app.render();const auto first=pixels();capture("merchant-flames-zero");
 app.map_clock_ms=2475;app.render();CHECK(differences(first,0,22,320,205)>100);capture("merchant-flames-2475ms");
 // At the old 5945 ms rounded directory boundary the original 6 s idle
 // must still show its last sample, rather than restarting either skin.
 app.map_clock_ms=5999;app.render();CHECK(app.content.image_cached(focused.first+144));
 for(int id:{AnimMerchantFlamesBehind,AnimMerchantFlamesFront}){auto clip=app.content.clip(id);CHECK(app.content.image_cached(clip.first+149));}
 app.map_clock_ms=6000;app.render();CHECK(app.content.image_cached(focused.first));
 for(int id:{AnimMerchantFlamesBehind,AnimMerchantFlamesFront})CHECK(app.content.image_cached(app.content.clip(id).first));
 app.fast_mode=true;app.tick_remainder=0;const auto before=app.map_clock_ms;app.advance(99);CHECK(app.map_clock_ms-before==99);app.render();unchanged();app.fast_mode=false;
 CHECK(app.content.healthy()&&app.content.image_cache_usage().pages<=748&&app.content.image_cache_usage().entries<=320);
}
static void rejected_profiles(Bytes&data){
 const uint32_t fields[][3]={{4,20,19},{28,1702,1705},{28,1704,1705},{32,557,558}};
 for(const auto&field:fields){put(data,field[0],field[1]);CHECK(!app.content.open(data.data(),data.size()));put(data,field[0],field[2]);}
 CHECK(app.content.open(data.data(),data.size())&&app.content.current_profile());
}
int main(int argc,char**argv){
 if(argc<3||argc>4){std::fprintf(stderr,"usage: merchant_visual_regression v19.nsp v18.nsp [private-screenshot-dir]\n");return 2;}
 if(argc==4)screenshots=argv[3];auto candidate=read(argv[1]);CHECK(app.init(candidate.data(),candidate.size(),{}));
 CHECK(app.content.version()==MerchantEffectsContentVersion&&app.content.current_profile());phase_and_focus();rejected_profiles(candidate);
 auto legacy=read(argv[2]);CHECK(app.init(legacy.data(),legacy.size(),{}));CHECK(app.content.version()==FeedbackContentVersion&&app.content.current_profile());
 CHECK(!app.content.clip(AnimMerchantRoomFocused).count&&!app.content.clip(AnimMerchantFlamesBehind).count);
 shop(Character::Ironclad);app.input(Key::Left);app.render();capture("merchant-v18-fallback");unchanged();
 app.input(Key::Confirm);CHECK(app.shop_open);app.input(Key::Back);CHECK(!app.shop_open);unchanged();
 CHECK(app.content.healthy());std::printf("merchant visual: %u checks passed; exact v19 profile, phase/focus, five classes, v18 fallback\n",checks);
}
