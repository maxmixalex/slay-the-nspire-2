// Rendering safety and original trigger coverage. Private-asset mode samples
// the installed textures; neither mode is calculator performance evidence.
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
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"room secondary VFX: line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static std::vector<uint8_t> read(const std::string&path){
 FILE*f=std::fopen(path.c_str(),"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long n=std::ftell(f);CHECK(n>0);CHECK(!std::fseek(f,0,SEEK_SET));
 std::vector<uint8_t>data(size_t(n),0);CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));return data;
}
static void fresh(){
 new_run(a.game,0x524f4f4d,false,Character::Ironclad);init_world_relics(a.game);a.game.phase=Phase::Map;
 a.title=a.paused=a.help=a.library=a.character_select=a.settings_open=false;a.progress.tutorial_flags=0;a.reset_presentation();
}
static void save_before(){CHECK(validate(a.game));CHECK(save(a.game,before));}
static void unchanged(){CHECK(save(a.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static void fill(){for(int y=0;y<240;++y)for(int x=0;x<320;++x)a.pixels[y*320+x]=rgb(24+x%88,28+y%76,32+(x+y)%80);}
static unsigned differences(const std::vector<uint16_t>&original,int first=0,int last=240){unsigned result=0;for(int y=first;y<last;++y)for(int x=0;x<320;++x)result+=a.pixels[y*320+x]!=original[y*320+x];return result;}
static std::vector<uint16_t> pixels(){return {a.pixels,a.pixels+320*240};}
static void tone_clip(){
 fresh();Canvas c{a};save_before();
 for(unsigned age:{0u,500u,1500u,2500u,3000u,0xffffffffu}){
  fill();const auto original=pixels();room_vfx_rest_tone(c,age);CHECK(!differences(original,0,22));
  CHECK(bool(differences(original,22))==(age>0&&age<3000));unchanged();
 }
}
static void actual_masks(){
 Canvas c{a};fresh();save_before();
 fill();auto original=pixels();room_vfx_smoke(c,160,202,320,1000,12,0x4845414c);CHECK(differences(original,22)>0);CHECK(!differences(original,0,22));unchanged();
 fill();original=pixels();room_vfx_smoke(c,160,202,320,4000,12,0x4845414c);CHECK(!differences(original));unchanged();
 // Culling must be pixel-identical after the rug overlays the complete room,
 // including a candle's drifting specks outside its smaller light rectangle.
 for(uint32_t now:{100u,1450u,4000000000u}){
  a.map_clock_ms=now;fill();merchant_secondary_lights(c,false);c.ui(U_MerchantRug,MerchantInventoryLayout::RugX,MerchantInventoryLayout::RugY,MerchantInventoryLayout::RugW,MerchantInventoryLayout::RugH);const auto full=pixels();
  fill();merchant_secondary_lights(c,true);c.ui(U_MerchantRug,MerchantInventoryLayout::RugX,MerchantInventoryLayout::RugY,MerchantInventoryLayout::RugW,MerchantInventoryLayout::RugH);CHECK(!differences(full));unchanged();
 }
 fresh();a.map_clock_ms=200;a.reward_presentation.append(RewardPickupKind::Card,Strike,0,90,120,0,{Strike,0});save_before();
 fill();original=pixels();reward_card_trail(c,a.reward_presentation.flights[0],266,11);CHECK(differences(original)>0);unchanged();
 for(auto kind:{RewardPickupKind::Potion,RewardPickupKind::Relic}){
  fresh();a.reward_presentation.append(kind,kind==RewardPickupKind::Potion?unsigned(Potion::Strength):unsigned(WorldRelicBurningBlood),0,80,110,0);a.map_clock_ms=500;save_before();
  CHECK(!a.reward_presentation.hidden(kind,0,a.map_clock_ms));fill();original=pixels();reward_inventory_flash(c,a.reward_presentation.flights[0],130,11);CHECK(differences(original)>0);unchanged();
  a.map_clock_ms=3000;fill();original=pixels();reward_inventory_flash(c,a.reward_presentation.flights[0],130,11);CHECK(!differences(original));unchanged();
 }
}
static void actual_heal_and_treasure(){
 fresh();CHECK(world_begin_run(a.game,reference::ActKind::Overgrowth,0));a.game.original_events=OriginalEventState{};
 auto&w=a.game.world;w.room_type=uint8_t(WorldRoom::Rest);w.room_open=1;w.rest_count=1;w.rest_options[0]=uint8_t(WorldRestOption::Heal);w.rest_used=0;w.rest_pending=0;
 a.game.phase=Phase::Camp;a.game.hp=30;a.map_clock_ms=1000;a.cursor=0;
 CHECK(a.camp_input(Key::Confirm));CHECK(a.game.hp>30);CHECK(a.room_presentation.rest.healing);save_before();
 Canvas c{a};a.map_clock_ms+=1000;fill();native_camp_screen(c);unchanged();
 // A restored completed rest must not replay Heal; its canonical bytes have
 // no transient particle clock and reset_presentation clears that clock.
 a.reset_presentation();a.sync_camp_presentation();CHECK(!a.room_presentation.rest.healing);unchanged();
 fresh();CHECK(world_begin_run(a.game,reference::ActKind::Overgrowth,0));a.game.original_events=OriginalEventState{};
 auto&t=a.game.world;t.room_type=uint8_t(WorldRoom::Treasure);t.room_open=1;t.treasure_relic=WorldRelicAnchor;a.game.phase=Phase::Treasure;a.game.treasure_state=0;a.cursor=0;
 a.animation=100;a.map_clock_ms=3300;a.treasure_action(UiAction::TreasureOpen);CHECK(a.game.treasure_state==1&&t.treasure_gold>0);save_before();
 a.animation+=15;fill();const auto original=pixels();treasure_secondary_coins(c);CHECK(differences(original,22)>0);CHECK(!differences(original,0,22));unchanged();
 a.reset_treasure_presentation();fill();treasure_secondary_coins(c);CHECK(!differences(original));unchanged();
}
int main(int argc,char**argv){
 CHECK(argc==1||argc==2);tone_clip();
 if(argc==2){const std::string root=argv[1];const auto common=read(root+"/nSpire2.common.tns"),language=read(root+"/nSpire2.eng.tns");
  SplitResourceSource split;CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(language.data(),language.size()),ResourceLanguage::English));
  CHECK(a.init(split.source(),{}));actual_masks();actual_heal_and_treasure();CHECK(a.content.healthy());
 }
 std::printf("room secondary VFX: %u checks passed%s\n",checks,argc==2?" with original installed assets":" (asset-independent safety)");
}
