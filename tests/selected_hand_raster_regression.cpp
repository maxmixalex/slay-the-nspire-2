// Independent before/after transcript for the selected-hand raster lifetime.
// Optional actual packs are private inputs; no original pixels are committed.
#include "../src/app.cpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App a;
static State combat,shop;
static SaveData before,after,disk;
static unsigned checks=0,frames=0;
static const char*stage="initial";
static FILE*capture=nullptr;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"selected raster [%s]:%d %s\n",stage,__LINE__,#x);std::exit(1);}}while(0)
struct Input {
 std::vector<uint8_t> bytes;
 uint64_t calls=0,count=0,trace=1469598103934665603ull;
 static bool read(void*context,uint32_t at,void*out,uint32_t n){
  auto&s=*static_cast<Input*>(context);
  if(at>s.bytes.size()||n>s.bytes.size()-at||(n&&!out))return false;
  ++s.calls;s.count+=n;s.trace=(s.trace^at)*1099511628211ull;s.trace=(s.trace^n)*1099511628211ull;
  std::memcpy(out,s.bytes.data()+at,n);return true;
 }
 ResourceSource source(){return {this,uint32_t(bytes.size()),read};}
};
static uint64_t hash(const void*data,size_t n){
 const auto*p=static_cast<const uint8_t*>(data);uint64_t result=1469598103934665603ull;
 for(size_t i=0;i<n;++i)result=(result^p[i])*1099511628211ull;return result;
}
static Input file(const char*path){
 FILE*f=std::fopen(path,"rb");CHECK(f&&!std::fseek(f,0,SEEK_END));const long n=std::ftell(f);
 CHECK(n>40&&!std::fseek(f,0,SEEK_SET));Input result;result.bytes.resize(size_t(n));
 CHECK(std::fread(result.bytes.data(),1,result.bytes.size(),f)==result.bytes.size());CHECK(!std::fclose(f));return result;
}
static bool read_saved(State&state){return load(state,disk.bytes,disk.size);}
static void record(Input&input){
 CHECK(save(a.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
 CHECK(a.content.healthy());
 std::printf("frame %u %s %u %u %016llx %016llx %llu %llu %016llx %u %d %d %d\n",frames++,stage,a.content.language(),a.map_clock_ms,
  static_cast<unsigned long long>(hash(a.pixels,sizeof a.pixels)),static_cast<unsigned long long>(hash(after.bytes,after.size)),
  static_cast<unsigned long long>(input.calls),static_cast<unsigned long long>(input.count),static_cast<unsigned long long>(input.trace),
  selected_hand_reader.started,selected_hand_reader.instance,selected_hand_reader.skip,selected_hand_reader.maximum);
 if(capture)CHECK(std::fwrite(a.pixels,1,sizeof a.pixels,capture)==sizeof a.pixels);
}
static void render(Input&input,unsigned now){a.map_clock_ms=now;CHECK(save(a.game,before));a.render();record(input);}
static void selected_setup(){
 new_run(a.game,0x41cef1ea,false,Character::Ironclad);a.game.phase=Phase::Combat;
 a.game.current_node=a.game.floor=a.game.turn=1;enter_combat(a.game);CHECK(encounter_setup(a.game,EncounterId::HauntedShipNormal));
 a.game.card_count=a.game.hand.count=3;a.game.draw.count=a.game.discard.count=a.game.exhaust.count=0;
 const CardKind cards[]={Rage,Strike,Defend};
 for(unsigned i=0;i<3;++i){a.game.cards[i]={cards[i],0};a.game.zones[i]=Zone::Hand;a.game.hand.ids[i]=uint16_t(i);}
 a.game.expansion.last_drawn=-1;for(auto&origin:a.game.event_cards.deck_origin)origin=0;
 a.game.deck[0]={Rage,0};a.game.energy=20;++a.game.revision;
 a.title=a.paused=a.help=a.library=a.character_select=a.settings_open=a.profiles_open=a.compendium_open=false;
 a.statistics_open=a.history_open=a.map_preview.open=false;a.run_mode={};a.frontend_confirm=0;a.progress.tutorial_flags=0;
 a.map_clock_ms=10000;a.animation=300;a.reset_presentation();a.pointer.visible=false;
 a.sync_combat_preparation();a.combat_preparation.shown=true;
 for(unsigned i=0;i<1024&&a.combat_preparation.active;++i)a.step_combat_preparation();CHECK(!a.combat_preparation.active);
 a.sync_combat_feedback();a.sync_enemy_feedback();a.hand_focus=true;a.cursor=0;a.keyboard_card=-1;
 CHECK(validate(a.game));
}
static void scroll(Input&input,const char*name){
 stage=name;render(input,a.map_clock_ms);CHECK(selected_hand_reader.maximum>0&&selected_hand_reader.instance==0);
 const auto start=selected_hand_reader.started;const unsigned travel=unsigned(selected_hand_reader.maximum)*125u;
 for(unsigned age:{0u,1399u,1400u,1525u,1650u,1400u+travel,2799u+travel,2925u+travel,2800u+2*travel}){
  render(input,start+age);CHECK(selected_hand_reader.started==start);
 }
}
static void scenes(Input&input){
 stage="selected-before-scenes";render(input,a.map_clock_ms+1525);const auto started=selected_hand_reader.started;
 const auto source=selected_hand_reader.source;const auto built=selected_hand_reader.built;combat=a.game;
 const unsigned clock=a.map_clock_ms;
 stage="random";a.character_select=a.character_random=true;
 for(unsigned i=0;i<3;++i)render(input,clock+33*(i+1));
 stage="shop";a.character_select=false;a.game=shop;a.shop_open=true;
 a.room_presentation.merchant.set_open(true,clock-1000);a.merchant_preparation.planned=true;
 for(unsigned i=0;i<3;++i)render(input,clock+132+33*i);
 stage="selected-after-scenes";a.game=combat;a.shop_open=false;
 for(unsigned i=0;i<12;++i)render(input,clock+250+33*i);
 CHECK(selected_hand_reader.source==source&&selected_hand_reader.built==built&&selected_hand_reader.started==started);
}
static void dragging(Input&input){
 stage="selected-drag";a.pointer.visible=true;render(input,a.map_clock_ms+33);
 const auto pose=a.combat_hand_pose(0,true);a.pointer_move(pose.cx,mn(235,pose.cy));
 a.pointer_button(true);a.pointer_move(160,105);CHECK(a.pointer.held&&a.pointer.dragging&&a.selected_hand_card()==0);
 for(unsigned i=0;i<8;++i){a.pointer_move(160+int(i)*3,105-int(i));render(input,a.map_clock_ms+125);}
 // Cancel the gesture without playing a card or changing the canonical run.
 a.pointer_cancel();a.pointer.visible=false;a.hand_focus=false;
 stage="no-focused-hand";render(input,a.map_clock_ms+33);CHECK(selected_hand_reader.instance<0);
 a.hand_focus=true;a.cursor=0;stage="refocused-hand";render(input,a.map_clock_ms+33);CHECK(selected_hand_reader.instance==0);
}
static void run(Input&input){
 CHECK(a.init(input.source(),{nullptr,read_saved}));selected_setup();
 stage="fresh-selected";scroll(input,"scroll");dragging(input);scenes(input);scroll(input,"scroll-after-scenes");
 stage="console";a.input(Key::Debug);render(input,a.map_clock_ms+33);CHECK(a.debug_open);
 a.input(Key::Back);render(input,a.map_clock_ms+33);CHECK(!a.debug_open);
 // Init/reopen must invalidate the former raster even with the same App and
 // Content addresses. The native callback uses the real serialized save.
 CHECK(save(a.game,disk));stage="native-reopen";CHECK(a.init(input.source(),{nullptr,read_saved}));
 CHECK(selected_hand_reader.instance<0&&load(a.game,disk.bytes,disk.size));
 a.title=false;a.reset_presentation();a.hand_focus=true;a.cursor=0;a.map_clock_ms=50000;a.animation=1500;
 a.sync_combat_preparation();a.combat_preparation.shown=true;
 for(unsigned i=0;i<1024&&a.combat_preparation.active;++i)a.step_combat_preparation();CHECK(!a.combat_preparation.active);
 scroll(input,"reopened-scroll");
}
static void independent_metadata(){
 stage="shared-owner invalidation";Canvas c{a};a.hand_focus=true;a.cursor=0;
 CHECK(selected_hand_tile(c,0).description_truncated&&scene_raster_owner==SceneRasterOwner::SelectedHand);
 const auto started=selected_hand_reader.started;
 const auto*rug=merchant_cached_rug(c);CHECK(rug&&scene_raster_owner==SceneRasterOwner::Merchant);
 const auto pixels=hash(rug->pixels,sizeof rug->pixels),alpha=hash(rug->alpha,sizeof rug->alpha);
 invalidate_hand_tiles();CHECK(selected_hand_reader.instance<0&&selected_hand_reader.started==started);
 CHECK(scene_raster_owner==SceneRasterOwner::Merchant&&pixels==hash(rug->pixels,sizeof rug->pixels)&&alpha==hash(rug->alpha,sizeof rug->alpha));
 a.hand_focus=false;a.pointer_cancel();combat_screen(c);
 CHECK(selected_hand_reader.instance<0&&scene_raster_owner==SceneRasterOwner::Merchant);
 CHECK(pixels==hash(rug->pixels,sizeof rug->pixels)&&alpha==hash(rug->alpha,sizeof rug->alpha));
 a.hand_focus=true;CHECK(selected_hand_tile(c,0).description_truncated&&scene_raster_owner==SceneRasterOwner::SelectedHand);
}
int main(int argc,char**argv){
 CHECK(argc==1||argc==3||argc==4);invalidate_hand_tiles();CHECK(selected_hand_reader.instance<0);
 if(argc==1){std::printf("selected raster: %u reset checks; actual packs required for frame gate\n",checks);return 0;}
 auto eng=file(argv[1]),zhs=file(argv[2]);
 if(argc==4){capture=std::fopen(argv[3],"wb");CHECK(capture);}
 new_run(shop,73,false);shop.map[0].room=Room::Shop;CHECK(choose_node(shop,0));
 run(eng);stage="language-reload";a.title=true;CHECK(a.reload_language(zhs.source()));
 CHECK(selected_hand_reader.instance<0);selected_setup();scroll(zhs,"chinese-reloaded-scroll");scenes(zhs);dragging(zhs);
 run(zhs);stage="language-reload-english";a.title=true;CHECK(a.reload_language(eng.source()));
 CHECK(selected_hand_reader.instance<0);selected_setup();scroll(eng,"english-reloaded-scroll");
 independent_metadata();
 if(capture)CHECK(!std::fclose(capture));
 std::fprintf(stderr,"selected raster: %u checks, %u complete frames\n",checks,frames);
}
