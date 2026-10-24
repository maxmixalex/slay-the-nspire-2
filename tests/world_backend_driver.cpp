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
enum class SceneKind { Neow,Map,MapPointer,MapFast,Combat,Kaiser,FakeMerchant,Crystal,Colossal,IntroNormal,IntroInterrupted,IntroTopBar,IntroFast,Rest,TreasureKey,TreasurePointer,TreasureReload,TreasureDolly,TreasureEmpty };
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
 {"world-colossal",SceneKind::Colossal},
 {"world-intro-normal",SceneKind::IntroNormal},{"world-intro-interrupted",SceneKind::IntroInterrupted},
 {"world-intro-top-bar",SceneKind::IntroTopBar},{"world-intro-fast",SceneKind::IntroFast},
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
static bool intro_scene(SceneKind kind){return kind>=SceneKind::IntroNormal&&kind<=SceneKind::IntroFast;}
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
 if(kind==SceneKind::Neow||intro_scene(kind))return;
 // Explicit supplied Neow outcome isolates the map and following room UI.
 // The Neow input/capture sequence is independently retained as case zero.
 initial.original_events=OriginalEventState{};
 if(kind==SceneKind::FakeMerchant||kind==SceneKind::Crystal||kind==SceneKind::Colossal){
  initial.gold=500;
  const auto event=kind==SceneKind::FakeMerchant?OriginalEventId::FakeMerchant:kind==SceneKind::Crystal?OriginalEventId::CrystalSphere:OriginalEventId::ColossalFlower;
  require(original_event_begin(initial,event,17,0),"prepare native event");
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
static void advance_for(unsigned remaining){while(remaining){const unsigned step=remaining>250?250:remaining;advance(step);remaining-=step;}}
static void settle_preparation(){
 for(unsigned step=0;app.combat_preparation.active&&step<1024;++step)advance(33);
 require(!app.combat_preparation.active,"actual battle resources finish their bounded preparation");
}
static void intro_steps(SceneKind kind){
 // An empty fixture gives both production hosts a fresh title. Start the
 // actual deterministic default run through its UI; loading Neow would
 // intentionally restore settled presentation and skip this introduction.
 require(app.title&&!app.has_save,"intro starts without a saved run");
 if(kind==SceneKind::IntroFast){
  click(HitKind::Menu,1);action(UiAction::SettingsFastMode);action(UiAction::SettingsBack);
  require(app.fast_mode&&!app.settings_open,"fresh run uses real Fast settings");
 }
 click(HitKind::Menu,0);require(app.character_select,"new run opens character selection");action(UiAction::StartCharacter);
 require(!app.title&&app.game.original_events.id==OriginalEventId::Neow&&!app.map_act_intro.played,"fresh Neow retains first-open eligibility");
 if(kind==SceneKind::IntroTopBar){
  require(save(app.game,map_before),"save unfinished Neow before top-bar inspection");key(Key::Map);
  require(app.map_act_intro.played&&!app.map_act_intro.camera_running&&app.map_act_intro.banner_count==1,"top-bar first-open skips the sweep but shows its banner");
  advance_for(1250);same_map_state(map_before);key(Key::Map);advance_for(350);
  require(!app.map_window.visible,"top-bar map closes after its tail");same_map_state(map_before);
 }
 require(app.game.original_events.offers[0]==WorldRelicFishingRod,"default run selects the known nonmodal Neow offer");
 click(HitKind::Choice,0);require(app.game.original_events.finished&&!world_relic_pending(app.game),"actual Neow acquisition finishes without fixture writes");
 click(HitKind::Choice,0);require(app.game.world.room_open==2&&app.map_window.is_open,"real Neow Proceed opens the retained-room map");
 require(save(app.game,map_before),"save canonical departure before act presentation");
 if(kind==SceneKind::IntroNormal){
  require(app.map_act_intro.camera_running&&app.map_act_intro.banner_count==1,"ordinary first Proceed starts camera and banner");
  unsigned previous=0;
  for(unsigned time:{125u,1000u,1749u,1750u,2500u,3999u,4000u,4750u}){
   advance_for(time-previous);previous=time;same_map_state(map_before);
   if(time==1749)require(!app.map_act_intro.interruptible,"normal sweep cannot interrupt early");
   if(time==1750)require(app.map_act_intro.interruptible,"normal sweep opens authored interrupt window");
   if(time==4000)require(!app.map_act_intro.camera_running&&!app.map_act_intro.y_q12,"camera reaches its original endpoint");
  }
  require(!app.map_act_intro.banner_count,"normal banner completes its independent 4750 ms lifetime");
 }else if(kind==SceneKind::IntroInterrupted){
  advance_for(1250);key(Key::Map);advance_for(350);
  require(!app.map_window.visible&&app.map_act_intro.camera_running,"closing keeps an unfinished camera for reopening");
  const unsigned elapsed=app.map_act_intro.camera_elapsed,banner=app.map_act_intro.banners[0].elapsed;
  advance_for(750);require(app.map_act_intro.camera_elapsed==elapsed&&app.map_act_intro.banners[0].elapsed==banner,"fully hidden map pauses camera and banner");
  key(Key::Map);require(app.map_act_intro.camera_elapsed==elapsed,"reopening does not consume hidden elapsed time");
  require(elapsed<1750,"close-tail sample precedes first interrupt");advance_for(1750-elapsed);
  key(Key::Up);require(!app.map_act_intro.camera_running&&app.map_act_intro.input_guard,"real scroll interrupts and starts the separate input guard");
  advance_for(199);key(Key::Up);require(app.map_act_intro.input_guard&&!app.map_scroll,"guard prevents early scrolling");
  advance(1);require(!app.map_act_intro.input_guard,"input guard releases at 200 ms");advance(67);
  require(!app.map_act_intro.y_q12,"interrupted camera settles through its lerp");same_map_state(map_before);
 }else{
  require(app.map_act_intro.played&&!app.map_act_intro.camera_running,"Fast or prior top-bar open does not replay the sweep");
  advance_for(kind==SceneKind::IntroFast?3250:4750);require(!app.map_act_intro.banner_count,"skipped sweep retains the independent banner lifetime");same_map_state(map_before);
 }
 key(Key::Save);key(Key::Load);same_map_state(map_before);
 require(app.map_act_intro.played&&!app.map_act_intro.camera_running&&!app.map_act_intro.banner_count,"canonical load restores settled act presentation");
}
static void colossal_steps(){
 require(app.game.original_events.id==OriginalEventId::ColossalFlower,"actual Colossal Flower page is active");
 require(save(app.game,map_before),"snapshot original Colossal Flower state");
 // Three rows expose the pinned parameterized rainbow word in the actual
 // 320x240 layout. Replays use the ordinary scroll and elapsed-time exports.
 scroll(3);for(unsigned i=0;i<4;++i)advance(250);same_map_state(map_before);
 action(UiAction::ReadEvent);scroll(3);for(unsigned i=0;i<4;++i)advance(250);
 action(UiAction::CloseEventReader);same_map_state(map_before);key(Key::Save);key(Key::Load);same_map_state(map_before);
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
 if(app.combat_preparation.active){
  // Real packs can still be loading when the map's independent input timer
  // expires. Replay that visible loading path without forcing its cache or
  // flags. Exact fade callback boundaries are covered by the native map suite.
  key(Key::Map);require(!app.panel,"loading screen holds ordinary map input");
  settle_preparation();same_map_state(map_expected);advance_for(fast?350:800);
  key(Key::Map);require(app.panel==3,"map opens after the actual battle is ready");
  key(Key::Map);require(!app.panel,"ready battle map dismisses normally");
  advance_for(350);same_map_state(map_expected);key(Key::Save);key(Key::Load);same_map_state(map_expected);
  return;
 }
 if(fast){
  click(HitKind::Command,int(Key::Map));require(app.panel==3,"fast fade-in releases pointer immediately");
  key(Key::Map);require(app.panel==3,"fast fade-in retains keyboard lock");
  click(HitKind::Command,int(Key::Map));require(!app.panel,"pointer can dismiss map during fast keyboard lock");
 }
 advance(fast?250:199);key(Key::EndTurn);same_map_state(map_expected);
 advance(fast?50:1);require(!app.map_transition_blocks_keyboard(),"keyboard releases at authored fade-in callback");
 key(Key::Map);
 if(fast){
  // The pointer opened and closed the independent map at fade-in start.
  // Its original 350 ms close tail outlasts the 300 ms keyboard lock.
  require(!app.panel&&app.map_window.closing(),"released keyboard cannot reopen the still-closing map");
  advance(50);key(Key::Map);
 }
 require(app.panel==3,"map can reopen once its separate close tail has finished");
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
 require(!app.camp_proceed_ready(),"first Proceed waits for original local rest VFX");
 const unsigned wait=app.fast_mode?1500:2500;for(unsigned elapsed=0;elapsed+250<wait;elapsed+=250)advance(250);
 advance(249);require(!app.camp_proceed_ready(),"rest VFX remains pending at final millisecond");advance(1);require(app.camp_proceed_ready(),"rest VFX releases Proceed at source wait");
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
  world_fixture(scenario);require(save(initial,storage),"save controlled native room");if(intro_scene(kind))storage.size=0;
  require(app.init(pack.data(),pack.size(),{save_cb,load_cb}),"open actual world pack");write(directory+"/"+name+"-fixture.bin",storage.bytes,storage.size);
  std::fprintf(manifest,"%s{\"name\":\"%s\",\"steps\":[",index?",":"",name.c_str());
  if(intro_scene(kind))intro_steps(kind);
  else{
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
   settle_preparation();
   const int turn=app.game.turn;key(Key::EndTurn);require(app.game.turn==turn+1,"original enemy move executes through normal End Turn");
   for(unsigned i=0;i<40;i++)advance(250);
   key(Key::Statuses);key(Key::Back);key(Key::Save);key(Key::Load);advance(250);
  }else if(kind==SceneKind::FakeMerchant){
   action(UiAction::ReadEvent);scroll(1);action(UiAction::CloseEventReader);click(HitKind::Choice,0);key(Key::Save);click(HitKind::Choice,6);key(Key::Save);
  }else if(kind==SceneKind::Crystal){
   click(HitKind::Choice,1);require(app.game.phase==Phase::OriginalEventChoice,"Crystal enters its board choice");
   crystal_click_available();action(UiAction::CrystalLarge);crystal_click_available();key(Key::Save);key(Key::Load);
  }else if(kind==SceneKind::Colossal)colossal_steps();
  else if(kind==SceneKind::Rest)rest_steps();
  else treasure_steps(kind);
  }
  require(saved>0,"case exercises production save callback");std::fprintf(manifest,"],\"saves\":%u}",saved);total_frames+=frames;total_saves+=saved;
 }
 std::fputs("]}\n",manifest);require(!std::fclose(manifest),"close manifest");std::printf("Original-world backend fixtures: %u cases, %u frames, %u saves; no original-PC differential claim\n",count,total_frames,total_saves);
}
