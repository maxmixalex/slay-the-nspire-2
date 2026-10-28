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
 CHECK(card_smith_angle(0)==0&&card_smith_angle(250)==0);
 CHECK(card_smith_angle(400)==20*4096/360&&card_smith_angle(650)==-10*4096/360&&card_smith_angle(900)==5*4096/360);
 CHECK(smith_impact_frame(0)==0&&smith_impact_frame(55)==0&&smith_impact_frame(56)==1&&smith_impact_frame(499)==8);
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
  CHECK(!a.reward_presentation.hidden(kind,0,a.map_clock_ms));fill();original=pixels();reward_inventory_flash(c,a.reward_presentation.flights[0],130,11);
  // Relic holder callbacks own independent serial flashes now. A pickup
  // flight must not add a duplicate flash; potions keep their source effect.
  CHECK(bool(differences(original))==(kind==RewardPickupKind::Potion));unchanged();
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
 save_before();a.pointer.visible=true;a.pointer.x=319;a.pointer.y=239;fill();treasure_scene(c);const auto outside=pixels();
 a.cursor=1;fill();treasure_scene(c);CHECK(!differences(outside));unchanged();
 a.cursor=0;a.pointer.visible=false;fill();treasure_scene(c);CHECK(differences(outside)>0);const auto keyboard=pixels();
 a.pointer.visible=true;a.pointer.x=168;a.pointer.y=147;fill();treasure_scene(c);CHECK(!differences(keyboard));unchanged();
 a.pointer.visible=false;
 a.animation=100;a.map_clock_ms=3300;a.treasure_action(UiAction::TreasureOpen);CHECK(a.game.treasure_state==1&&t.treasure_gold>0);save_before();
 a.animation+=15;fill();const auto original=pixels();treasure_secondary_coins(c);CHECK(differences(original,22)>0);CHECK(!differences(original,0,22));unchanged();
 a.reset_treasure_presentation();fill();treasure_secondary_coins(c);CHECK(!differences(original));unchanged();
}
static void actual_smith(){
 fresh();Canvas c{a};a.game.phase=Phase::Camp;
 auto*card=a.reward_presentation.upgrades.add({Strike,1},{},0,1000,RewardPresentation::Upgrades::Smith);CHECK(card);card->energy=1;
 save_before();
 for(unsigned frame=0;frame<I15_SmithImpactCount;++frame){const auto p=a.content.image(a.content.ui_image(I15_SmithImpactStart+frame));CHECK(p.pixels&&p.w==120&&p.h==120);}
 for(unsigned age:{250u,400u,500u,650u,750u,900u,1100u,1249u}){
  a.map_clock_ms=1000+age;fill();const auto original=pixels();upgraded_card_animation(c);CHECK(differences(original)>0&&!card->launched);unchanged();
 }
 a.map_clock_ms=2250;fill();upgraded_card_animation(c);CHECK(card->launched);unchanged();
 unsigned flights=0;for(const auto&flight:a.reward_presentation.flights)if(flight.kind==RewardPickupKind::Card){++flights;CHECK(flight.start==2250&&flight.card.kind==Strike&&flight.card.upgraded);}
 CHECK(flights==1);upgraded_card_animation(c);flights=0;for(const auto&flight:a.reward_presentation.flights)flights+=flight.kind==RewardPickupKind::Card;CHECK(flights==1);unchanged();
}
static void actual_ancient_label(){
 fresh();CHECK(world_begin_run(a.game,reference::ActKind::Overgrowth,0));
 auto&e=a.game.original_events;e.finished=1;e.option_count=1;e.options[0]=0;
 std::strcpy(e.page_key,"DONE");std::strcpy(e.option_keys[0],"LEAVE");
 a.cursor=0;a.pointer.visible=false;a.text_effects=false;
 a.ancient_dialogue.option_heights[0]=24;a.room_presentation.event.buttons[0].observe(1,0);
 char description[128]{};native_event_option(a,0,true,description,sizeof description);CHECK(!*description);save_before();
 Canvas c{a};Canvas clipped=c;clipped.pixels+=ancient_layout::ClipY*c.pitch+ancient_layout::ClipX;
 clipped.clip_w=ancient_layout::ClipW;clipped.clip_h=ancient_layout::Bottom-ancient_layout::ClipY;
 // A settled hover remains still across the marquee pause. The historical
 //12px viewport fabricated one pixel of overflow for this single line.
 a.map_clock_ms=800;fill();ancient_option(c,clipped,0,210);const auto initial=pixels();
 a.map_clock_ms=2000;fill();ancient_option(c,clipped,0,210);CHECK(!differences(initial));unchanged();
 a.text_effects=true;
}
int main(int argc,char**argv){
 CHECK(argc==1||argc==2);tone_clip();
 if(argc==2){const std::string root=argv[1];const auto common=read(root+"/nSpire2.dat.tns"),language=read(root+"/nSpire2.eng.dat.tns");
  SplitResourceSource split;CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(language.data(),language.size()),ResourceLanguage::English));
  CHECK(a.init(split.source(),{}));actual_masks();actual_heal_and_treasure();actual_smith();actual_ancient_label();CHECK(a.content.healthy());
 }
 std::printf("room secondary VFX: %u checks passed%s\n",checks,argc==2?" with original installed assets":" (asset-independent safety)");
}
