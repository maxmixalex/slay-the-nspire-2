// A real cold inventory must acknowledge input before work and yield between
// decoded images. Native timing is not calculator performance evidence.
#include "../src/app.cpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App a;
static SaveData before,after;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"merchant preparation:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void unchanged(){CHECK(save(a.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static void shop(){
 new_run(a.game,73,false);a.game.map[0].room=Room::Shop;CHECK(choose_node(a.game,0));
 a.title=a.library=a.paused=a.help=a.character_select=a.settings_open=false;a.frontend_confirm=0;a.progress.tutorial_flags=0;
 a.reset_presentation();a.pointer.visible=false;a.map_clock_ms=1400;CHECK(save(a.game,before));
}
static bool action(UiAction value){for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::UiAction&&a.hits[i].value==int(value))return true;return false;}
static void pending_controls(){
 shop();auto&p=a.merchant_preparation;a.shop_open=true;p.active=true;p.card=ShopCardCount;
 a.step_merchant_preparation();CHECK(p.active&&!p.shown&&!p.updates);unchanged();
 a.input(Key::Confirm);CHECK(p.active&&a.shop_open);unchanged();
 a.input(Key::Back);CHECK(!p.active&&!a.shop_open);unchanged();
 p.active=p.shown=a.shop_open=true;p.card=ShopCardCount;a.paused=true;a.step_merchant_preparation();CHECK(p.active&&!p.updates);
 a.paused=false;a.step_merchant_preparation();CHECK(!p.active&&p.updates==1&&a.room_presentation.merchant.open);
 CHECK(a.room_presentation.merchant.start==a.map_clock_ms);unchanged();
}
static std::vector<uint8_t> read(const std::string&path){
 FILE*f=std::fopen(path.c_str(),"rb");CHECK(f&&!std::fseek(f,0,SEEK_END));const long length=std::ftell(f);CHECK(length>40&&!std::fseek(f,0,SEEK_SET));
 std::vector<uint8_t>b(static_cast<size_t>(length));CHECK(std::fread(b.data(),1,b.size(),f)==b.size());CHECK(!std::fclose(f));return b;
}
static void actual(const char*folder){
 const std::string root=folder;auto common=read(root+"/nSpire2.common.tns"),locale=read(root+"/nSpire2.eng.tns");SplitResourceSource split;
 CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(locale.data(),locale.size()),ResourceLanguage::English));CHECK(a.init(split.source(),{}));shop();a.render();
 const auto start=a.content.read_stats();a.ui_action(UiAction::OpenShop);auto&p=a.merchant_preparation;
 CHECK(a.shop_open&&p.active&&!p.shown&&p.count>0&&p.count<=96);
 CHECK(a.content.read_stats().callbacks==start.callbacks);a.step_merchant_preparation();CHECK(!p.updates);unchanged();
 a.render();CHECK(p.shown&&action(UiAction::CloseShop));
 for(int i=0;i<a.hit_count;++i)CHECK(a.hits[i].kind!=HitKind::Choice);unchanged();
 a.input(Key::Confirm);unchanged();a.input(Key::Back);CHECK(!p.active&&!a.shop_open);unchanged();
 a.ui_action(UiAction::OpenShop);CHECK(p.active);a.render();
 uint16_t backdrop[320*65];std::memcpy(backdrop,a.pixels+320*38,sizeof backdrop);
 unsigned steps=0;while(p.active&&steps<128){
  const unsigned decoded=p.cold_images,card=p.card;const uint32_t now=a.map_clock_ms;
  a.advance(33);++steps;CHECK(a.content.healthy()&&p.cold_images-decoded<=1&&p.card-card<=1);
  CHECK(a.map_clock_ms==now+33);unchanged();a.render();
  if(p.active)CHECK(!std::memcmp(backdrop,a.pixels+320*38,sizeof backdrop));
 }
 CHECK(steps>1&&steps<128&&!p.active&&p.cursor==p.count&&p.card==ShopCardCount);
 CHECK(a.shop_open&&a.room_presentation.merchant.open);
 // The source opening tween starts only when every required stage is ready.
 CHECK(a.room_presentation.merchant.start==a.map_clock_ms);
 for(unsigned i=0;i<40;++i){a.advance(33);a.render();}
 unsigned cards=0;for(int i=0;i<a.hit_count;++i)cards+=a.hits[i].kind==HitKind::Choice&&a.hits[i].value>=0&&a.hits[i].value<ShopCardCount;
 CHECK(cards==ShopCardCount);unchanged();CHECK(a.content.healthy());
 const unsigned decoded=p.cold_images;a.input(Key::Back);CHECK(!p.active&&!a.shop_open);unchanged();
 std::printf("actual cold inventory: %u image decodes in %u yielded updates; all seven stock slots visible\n",decoded,steps);
}
int main(int argc,char**argv){CHECK(argc==1||argc==2);pending_controls();if(argc==2)actual(argv[1]);std::printf("merchant preparation: %u checks passed\n",checks);}
