// Private actual-pack captures for native/WASM agreement. These controlled
// room fixtures are not captures of the original PC game or an AI oracle.
#include "app.hpp"
#include "content_fixture.hpp"
#include "original_events.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App app;
static State initial;
static SaveData storage;
static SaveData map_before,map_expected,map_actual;
static std::string directory,name;
static unsigned saved,frames;
static FILE*manifest;
enum class SceneKind { Neow,Map,MapPointer,MapFast,Combat,Kaiser,FakeMerchant,Crystal,Rest,TreasureKey,TreasurePointer,TreasureReload,TreasureDolly,TreasureEmpty };
struct Scenario {
 const char*name;SceneKind kind;
 Character character=Character::Ironclad;
 reference::ActKind region=reference::ActKind::Overgrowth;
 unsigned act=0;
};
static const Scenario scenarios[]={
 {"world-neow",SceneKind::Neow},{"world-map",SceneKind::Map},
 {"world-map-pointer",SceneKind::MapPointer},{"world-map-fast",SceneKind::MapFast},
 {"world-native-combat",SceneKind::Combat},{"world-kaiser",SceneKind::Kaiser},
 {"world-fake-merchant",SceneKind::FakeMerchant},{"world-crystal",SceneKind::Crystal},
 {"world-rest-ironclad",SceneKind::Rest},
 {"world-rest-silent",SceneKind::Rest,Character::Silent,reference::ActKind::Underdocks},
 {"world-rest-defect",SceneKind::Rest,Character::Defect,reference::ActKind::Overgrowth,1},
 {"world-rest-regent",SceneKind::Rest,Character::Regent,reference::ActKind::Overgrowth,2},
 {"world-rest-necrobinder",SceneKind::Rest,Character::Necrobinder},
 {"world-treasure-keyboard",SceneKind::TreasureKey},
 {"world-treasure-pointer",SceneKind::TreasurePointer,Character::Ironclad,reference::ActKind::Overgrowth,1},
 {"world-treasure-reload",SceneKind::TreasureReload,Character::Ironclad,reference::ActKind::Overgrowth,2},
 {"world-treasure-dolly",SceneKind::TreasureDolly},
 {"world-treasure-empty",SceneKind::TreasureEmpty}
};
static void require(bool value,const char*message){if(!value){std::fprintf(stderr,"%s frame %u: %s (phase %u)\n",name.c_str(),frames,message,unsigned(app.game.phase));std::exit(1);}}
static void write(const std::string&path,const void*data,size_t count){FILE*f=std::fopen(path.c_str(),"wb");require(f,"open output");require(std::fwrite(data,1,count,f)==count,"write output");require(!std::fclose(f),"close output");}
static bool save_cb(const State&s){if(!save(s,storage))return false;write(directory+"/"+name+"-save-"+std::to_string(saved++)+".bin",storage.bytes,storage.size);return true;}
static bool load_cb(State&s){return load(s,storage.bytes,storage.size);}
static void capture(){
 app.render();require(app.title||validate(app.game),"valid state after input");require(app.content.healthy(),"bounded content reader remains healthy");
 for(const auto&m:app.enemy_presentation)require(!m.overflow,"animation command queue did not overflow");
 write(directory+"/"+name+"-frame-"+std::to_string(frames++)+".rgb",app.pixels,sizeof app.pixels);
}
static void key(Key k){std::fprintf(manifest,"%s{\"key\":%u}",frames?",":"",unsigned(k));app.input(k);capture();}
static void advance(unsigned ms){std::fprintf(manifest,"%s{\"advance\":%u}",frames?",":"",ms);app.advance(ms);capture();}
static void move_pointer(int x,int y){
 std::fprintf(manifest,"%s{\"move\":[%d,%d]}",frames?",":"",x,y);app.pointer_move(x,y);capture();
}
static void pointer(int x,int y,bool pressed){
 std::fprintf(manifest,"%s{\"pointer\":[%d,%d,%s]}",frames?",":"",x,y,pressed?"true":"false");
 app.pointer_move(x,y);app.pointer_button(pressed);capture();
}
static void click(HitKind kind,int value){
 app.render();HitRegion chosen{};bool found=false;
 for(int i=0;i<app.hit_count;i++){const auto&h=app.hits[i];if(h.kind==kind&&h.value==value&&h.enabled){chosen=h;found=true;break;}}
 require(found,"requested visible enabled control exists");int x=chosen.x+chosen.w/2,y=chosen.y+chosen.h/2;
 const auto*top=app.hit_at(x,y);require(top&&top->kind==kind&&top->value==value,"control has an unobscured pointer target");
 std::fprintf(manifest,"%s{\"click\":[%d,%d]}",frames?",":"",x,y);
 app.pointer_move(x,y);app.pointer_button(true);app.pointer_button(false);capture();
}
static void action(UiAction value){click(HitKind::UiAction,int(value));}
static void crystal_click_available(){
 app.render();int cell=-1;
 for(int i=0;i<app.hit_count;i++){const auto&h=app.hits[i];if(h.kind==HitKind::CrystalCell&&h.enabled){cell=h.value;break;}}
 require(cell>=0,"original sphere has a still-covered clickable cell");
 // Its outer mask is already clear. Pick an actual enabled cell, then emit
 // the same real pointer event used by every other captured control.
 click(HitKind::CrystalCell,cell);
}
static void scroll(int rows){std::fprintf(manifest,"%s{\"scroll\":%d}",frames?",":"",rows);app.pointer_scroll(rows);capture();}
static void route_room(WorldRoom room,unsigned act){
 // Only unrelated room outcomes are supplied. The tested room is entered
 // through the generated map and its ordinary controller, as in room tests.
 for(unsigned guard=0;guard<120;guard++){
  if(initial.world.room_open&&initial.world.room_type==unsigned(room)&&initial.world.act_index==act)return;
  require(initial.phase!=Phase::Dead&&initial.phase!=Phase::Complete&&initial.phase!=Phase::Error,"route remains live");
  if(world_can_travel(initial)){
   uint8_t points[reference::Columns]{};require(world_available_points(initial,points,sizeof points)>0,"room route frontier");
   require(world_choose_point(initial,points[0]),"choose generated room route");
  }else{
   if(initial.original_events.active)initial.original_events=OriginalEventState{};
   if(world_room_is_combat(initial))initial.phase=Phase::Reward;
   require(world_finish_room(initial)&&world_valid_run(initial),"finish supplied unrelated room outcome");
  }
 }
 require(false,"controlled route reaches requested room");
}
static void world_fixture(const Scenario&scenario){
 const SceneKind kind=scenario.kind;const bool room=kind>=SceneKind::Rest;
 new_run(initial,0x41cef1ea,false,scenario.character);
 require(world_set_seed(initial,room?"NSPIRE-WORLD-017":"WORLD-BACKENDS-017"),"set native seed");
 require(world_begin_run(initial,scenario.region,0),"begin native run");
 if(room){
  if(kind==SceneKind::TreasureEmpty)require(gain_world_relic(initial,WorldRelicSilverCrucible)&&!world_relic_pending(initial),"supply Silver Crucible for empty treasure");
  route_room(kind==SceneKind::Rest?WorldRoom::Rest:WorldRoom::Treasure,scenario.act);
  if(kind==SceneKind::Rest)initial.hp=30;
  else if(kind==SceneKind::TreasureEmpty)require(initial.world.treasure_relic==NoWorldRelic,"Silver Crucible removes treasure offer");
  else{
   // Fixed model injection isolates acquisition and rendering. In
   // particular Dolly's shop rarity is not asserted to be a chest drop.
   initial.world.treasure_relic=kind==SceneKind::TreasureDolly?WorldRelicDollysMirror:WorldRelicVajra;
  }
  return;
 }
 if(kind==SceneKind::Neow)return;
 // Explicit supplied Neow outcome isolates the map and following room UI.
 // The Neow input/capture sequence is independently retained as case zero.
 initial.original_events=OriginalEventState{};
 if(kind==SceneKind::FakeMerchant||kind==SceneKind::Crystal){
  initial.gold=500;
  require(original_event_begin(initial,kind==SceneKind::FakeMerchant?OriginalEventId::FakeMerchant:OriginalEventId::CrystalSphere,17,0),"prepare native event");
  return;
 }
 require(world_finish_room(initial),"close supplied ancient outcome");
 if(kind==SceneKind::Map||kind==SceneKind::MapPointer||kind==SceneKind::MapFast)return;
 uint8_t points[reference::Columns]{};require(world_available_points(initial,points,sizeof points)>0,"native map frontier");
 require(world_choose_point(initial,points[0])&&initial.phase==Phase::Combat,"enter native first combat");
 require(encounter_setup(initial,kind==SceneKind::Combat?EncounterId::NibbitsWeak:EncounterId::KaiserCrabBoss,0),"supply controlled original encounter");
 initial.hp=initial.max_hp=999;
}
static void same_map_state(const SaveData&expected){
 require(save(app.game,map_actual),"encode map state after presentation");
 require(map_actual.size==expected.size&&!std::memcmp(map_actual.bytes,expected.bytes,expected.size),
         "map presentation preserves the complete canonical controller result");
}
static void map_steps(SceneKind kind){
 const bool fast=kind==SceneKind::MapFast;
 if(fast){
  // Change speed through the same visible controls the WASM replay uses.
  key(Key::Map);advance(250);advance(100);key(Key::Back);key(Key::Down);key(Key::Confirm);key(Key::Confirm);
  require(app.settings_open&&app.fast_mode,"real settings enable fast mode");
  key(Key::Back);key(Key::Back);require(!app.paused&&!app.settings_open,"resume room");key(Key::Map);advance(250);advance(200);
 }
 uint8_t points[reference::Columns]{};
 require(world_available_points(app.game,points,sizeof points)>0,"travel has a reachable point");
 const unsigned destination=points[0];const auto*map=world_current_map(app.game);
 require(map&&destination<map->count,"reachable destination belongs to current map");
 require(save(app.game,map_before),"encode pending departure");
 initial=app.game;require(world_choose_point(initial,destination)&&save(initial,map_expected),
                         "independent controller supplies entry result");
 if(kind==SceneKind::MapPointer)click(HitKind::WorldNode,int(destination));
 else{
  key(Key::Right);
  for(unsigned i=0;i<reference::Columns&&app.cursor!=map->points[destination].col;++i)key(Key::Right);
  require(app.cursor==map->points[destination].col,"keyboard selects destination");key(Key::Confirm);
 }
 require(app.map_travel.pending&&app.map_travel.to==destination,"selection begins delayed travel");
 const uint32_t start=app.map_clock_ms;const unsigned duration=fast?600:1100;
 key(Key::Confirm);key(Key::EndTurn);same_map_state(map_before);
 // Capture authored circle frame changes, path dots, marker and both fade
 // layers, rather than testing only the terminal room frame.
 advance(33);advance(51);advance(83);
 if(kind==SceneKind::MapPointer){
  click(HitKind::Command,int(Key::Inspect));
  require(app.panel==1&&app.map_travel.pending,"pointer deck overlay retains pending route");
 }
 while(uint32_t(app.map_clock_ms-start)<duration-1){
  const unsigned remaining=duration-1-uint32_t(app.map_clock_ms-start);
  advance(remaining>200?200:remaining);
 }
 require(app.map_travel.pending&&world_can_travel(app.game),"no early room entry");
 same_map_state(map_before);key(Key::Save);advance(1);
 require(!app.map_travel.pending&&app.map_travel.fading_in&&!app.panel&&!app.paused,
         "completed fade enters once and closes old overlays");
 same_map_state(map_expected);key(Key::EndTurn);same_map_state(map_expected);
 if(fast){
  click(HitKind::Command,int(Key::Map));require(app.panel==3,"fast fade-in releases pointer immediately");
  key(Key::Map);require(app.panel==3,"fast fade-in retains keyboard lock");
  click(HitKind::Command,int(Key::Map));require(!app.panel,"pointer can dismiss map during fast keyboard lock");
 }
 advance(fast?250:199);key(Key::EndTurn);same_map_state(map_expected);
 advance(fast?50:1);key(Key::Map);require(app.panel==3,"keyboard releases at authored fade-in callback");
 key(Key::Map);require(!app.panel,"dismiss map after callback");
 advance(250);advance(250);advance(100);same_map_state(map_expected);
 require(!app.map_travel.fading_in,"fade-in completes independently of input release");
 key(Key::Save);key(Key::Load);same_map_state(map_expected);
}
static void rest_steps(){
 require(app.game.phase==Phase::Camp&&!world_rest_can_proceed(app.game),"rest fixture starts lit");
 for(unsigned i=0;i<3;i++)advance(250);
 const int hp=app.game.hp;click(HitKind::Choice,0);
 require(app.game.phase==Phase::Camp&&app.game.hp>hp&&world_rest_can_proceed(app.game),"healing extinguishes rest scene");
 for(unsigned i=0;i<3;i++)advance(250);
 key(Key::Save);key(Key::Load);advance(250);
 require(app.game.phase==Phase::Camp&&!app.camp_options(nullptr,0),"rest completion survives reload");
 key(Key::Confirm);require(app.game.phase==Phase::Camp&&app.game.world.room_open==2&&app.map_window.is_open,"completed rest stays beneath the opened map");
}
static void treasure_steps(SceneKind kind){
 require(app.game.phase==Phase::Treasure&&!app.game.treasure_state,"treasure fixture starts closed");
 const unsigned relics=app.game.world_relics.count,deck=app.game.deck_count;
 key(Key::Confirm);require(app.game.phase==Phase::Treasure&&app.game.treasure_state==1,"open original treasure controller");
 if(kind==SceneKind::TreasureEmpty){
  require(!app.treasure_random&&!app.treasure_holder_delay_us,"empty collection consumes no cosmetic random draw");
  for(unsigned i=0;i<5;i++)advance(250);
  require(app.treasure_presentation==3&&app.game.world_relics.count==relics,"empty collection closes without a grant");
 }else{
  require(app.treasure_random&&app.treasure_holder_delay_us>=200000&&app.treasure_holder_delay_us<400000,"fresh holder draws a bounded independent delay");
  if(kind==SceneKind::TreasureKey){
   advance(66);require(!app.treasure_pointer_can_take(),"mouse remains ignored during early keyboard claim");
   key(Key::Confirm);
  }else if(kind==SceneKind::TreasurePointer){
   advance(66);require(!app.treasure_pointer_can_take(),"pointer is gated at press");pointer(160,110,true);
   for(unsigned i=0;i<4;i++)advance(250);
   require(app.treasure_pointer_can_take(),"pointer gate opens by one second");pointer(160,110,false);
   require(app.game.treasure_state==1&&app.game.world_relics.count==relics,"press begun before gate cannot acquire on release");
   action(UiAction::TreasureTake);
  }else if(kind==SceneKind::TreasureReload){
   advance(99);const uint32_t random=app.treasure_random;key(Key::Save);key(Key::Load);
   require(!app.treasure_presentation&&app.treasure_pointer_can_take()&&app.treasure_random==random,"restored offer is terminal without a random draw");
   action(UiAction::TreasureTake);
  }else{
   for(unsigned i=0;i<5;i++)advance(250);
   action(UiAction::TreasureTake);
  }
  require(app.game.treasure_state==2&&app.game.world_relics.count==relics+1,"treasure grants exactly one relic");
  if(kind==SceneKind::TreasureDolly){
   require(app.game.phase==Phase::WorldRelicChoice&&world_relic_pending(app.game),"Dolly opens its real acquisition continuation");
   key(Key::Save);key(Key::Load);advance(250);
   require(app.game.phase==Phase::WorldRelicChoice&&world_relic_pending(app.game),"Dolly pending selection survives canonical reload");
   click(HitKind::WorldChoice,0);action(UiAction::ConfirmWorldChoice);
   require(app.game.phase==Phase::Treasure&&!world_relic_pending(app.game)&&app.game.deck_count==deck+1,"Dolly returns after exactly one duplicate");
  }
  for(unsigned i=0;i<2;i++)advance(250);
 }
 key(Key::Save);key(Key::Load);key(Key::Confirm);
 require(app.game.phase==Phase::Treasure&&app.game.world.room_open==2&&app.map_window.is_open,"terminal treasure stays beneath the opened map");
}
int main(int argc,char**argv){
 if(argc!=3)return 2;directory=argv[2];FILE*f=std::fopen(argv[1],"rb");require(f,"open private content");
 std::vector<uint8_t>pack(spire_test::pack_size(f));require(std::fread(pack.data(),1,pack.size(),f)==pack.size(),"read complete private pack");require(!std::fclose(f),"close private content");
 manifest=std::fopen((directory+"/scenarios.json").c_str(),"w");require(manifest,"open manifest");std::fputs("{\"cases\":[",manifest);
 unsigned total_frames=0,total_saves=0;
 constexpr unsigned count=sizeof scenarios/sizeof scenarios[0];
 for(unsigned index=0;index<count;index++){
  const auto&scenario=scenarios[index];const SceneKind kind=scenario.kind;
  // Each WASM scenario starts a fresh instance. Match its session-local
  // animation clock and cosmetic random stream with a fresh native App.
  name=scenario.name;saved=frames=0;app=App{};
  world_fixture(scenario);require(save(initial,storage),"save controlled native room");require(app.init(pack.data(),pack.size(),{save_cb,load_cb}),"open actual world pack");write(directory+"/"+name+"-fixture.bin",storage.bytes,storage.size);
  std::fprintf(manifest,"%s{\"name\":\"%s\",\"steps\":[",index?",":"",name.c_str());
  key(Key::Confirm);require(!app.title,"load native fixture from title");
  if(kind==SceneKind::Neow){action(UiAction::ReadEvent);scroll(1);action(UiAction::CloseEventReader);key(Key::Save);}
  else if(kind==SceneKind::Map||kind==SceneKind::MapPointer||kind==SceneKind::MapFast){
   key(Key::Map);advance(250);advance(200);
   if(kind==SceneKind::Map){
    key(Key::Detail);key(Key::Down);key(Key::Back);
    // Original directional reticles retain independently fading old focus.
    // Mouse mode cancels them; a later direction and load reestablish focus
    // through the real App input/lifecycle paths, without fixture writes.
    key(Key::Right);advance(33);key(Key::Left);advance(99);
    move_pointer(10,210);advance(231);key(Key::Right);advance(66);
    key(Key::Save);key(Key::Load);key(Key::Map);advance(250);advance(200);
   }
   map_steps(kind);
  }
  else if(kind==SceneKind::Combat||kind==SceneKind::Kaiser){
   const int turn=app.game.turn;key(Key::EndTurn);require(app.game.turn==turn+1,"original enemy move executes through normal End Turn");
   for(unsigned i=0;i<40;i++)advance(250);
   key(Key::Statuses);key(Key::Back);key(Key::Save);key(Key::Load);advance(250);
  }else if(kind==SceneKind::FakeMerchant){
   action(UiAction::ReadEvent);scroll(1);action(UiAction::CloseEventReader);click(HitKind::Choice,0);key(Key::Save);click(HitKind::Choice,6);key(Key::Save);
  }else if(kind==SceneKind::Crystal){
   click(HitKind::Choice,1);require(app.game.phase==Phase::OriginalEventChoice,"Crystal enters its board choice");
   crystal_click_available();action(UiAction::CrystalLarge);crystal_click_available();key(Key::Save);key(Key::Load);
  }else if(kind==SceneKind::Rest)rest_steps();
  else treasure_steps(kind);
  require(saved>0,"case exercises production save callback");std::fprintf(manifest,"],\"saves\":%u}",saved);total_frames+=frames;total_saves+=saved;
 }
 std::fputs("]}\n",manifest);require(!std::fclose(manifest),"close manifest");std::printf("Original-world backend fixtures: %u cases, %u frames, %u saves; no original-PC differential claim\n",count,total_frames,total_saves);
}
