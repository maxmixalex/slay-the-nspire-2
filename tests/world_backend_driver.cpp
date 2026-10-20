// Private actual-pack captures for native/WASM agreement. These controlled
// room fixtures are not captures of the original PC game or an AI oracle.
#include "app.hpp"
#include "content_fixture.hpp"
#include "original_events.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
using namespace spire;
static App app;
static State initial;
static SaveData storage;
static std::string directory,name;
static unsigned saved,frames;
static FILE*manifest;
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
static void click(HitKind kind,int value){
 app.render();HitRegion chosen{};bool found=false;
 for(int i=0;i<app.hit_count;i++){const auto&h=app.hits[i];if(h.kind==kind&&h.value==value&&h.enabled){chosen=h;found=true;break;}}
 require(found,"requested visible enabled control exists");int x=chosen.x+chosen.w/2,y=chosen.y+chosen.h/2;
 const auto*top=app.hit_at(x,y);require(top&&top->kind==kind&&top->value==value,"control has an unobscured pointer target");
 std::fprintf(manifest,"%s{\"click\":[%d,%d]}",frames?",":"",x,y);
 app.pointer_move(x,y);app.pointer_button(true);app.pointer_button(false);capture();
}
static void action(UiAction value){click(HitKind::UiAction,int(value));}
static void scroll(int rows){std::fprintf(manifest,"%s{\"scroll\":%d}",frames?",":"",rows);app.pointer_scroll(rows);capture();}
static void world_fixture(unsigned index){
 new_run(initial,0x41cef1ea,false,Character::Ironclad);
 require(world_set_seed(initial,"WORLD-BACKENDS-017"),"set native seed");
 require(world_begin_run(initial,reference::ActKind::Overgrowth,0),"begin native run");
 if(index==0)return;
 // Explicit supplied Neow outcome isolates the map and following room UI.
 // The Neow input/capture sequence is independently retained as case zero.
 initial.original_events=OriginalEventState{};
 if(index>=4){
  initial.gold=500;
  require(original_event_begin(initial,index==4?OriginalEventId::FakeMerchant:OriginalEventId::CrystalSphere,17,0),"prepare native event");
  return;
 }
 require(world_finish_room(initial),"close supplied ancient outcome");
 if(index==1)return;
 uint8_t points[reference::Columns]{};require(world_available_points(initial,points,sizeof points)>0,"native map frontier");
 require(world_choose_point(initial,points[0])&&initial.phase==Phase::Combat,"enter native first combat");
 require(encounter_setup(initial,index==2?EncounterId::NibbitsWeak:EncounterId::KaiserCrabBoss,0),"supply controlled original encounter");
 initial.hp=initial.max_hp=999;
}
int main(int argc,char**argv){
 if(argc!=3)return 2;directory=argv[2];FILE*f=std::fopen(argv[1],"rb");require(f,"open private content");
 std::vector<uint8_t>pack(spire_test::pack_size(f));require(std::fread(pack.data(),1,pack.size(),f)==pack.size(),"read complete private pack");require(!std::fclose(f),"close private content");
 const char*names[]={"world-neow","world-map","world-native-combat","world-kaiser","world-fake-merchant","world-crystal"};
 manifest=std::fopen((directory+"/scenarios.json").c_str(),"w");require(manifest,"open manifest");std::fputs("{\"cases\":[",manifest);
 unsigned total_frames=0,total_saves=0;
 for(unsigned index=0;index<6;index++){
  name=names[index];saved=frames=0;app=App{};
  world_fixture(index);require(save(initial,storage),"save controlled native room");require(app.init(pack.data(),pack.size(),{save_cb,load_cb}),"open actual world pack");write(directory+"/"+name+"-fixture.bin",storage.bytes,storage.size);
  std::fprintf(manifest,"%s{\"name\":\"%s\",\"steps\":[",index?",":"",name.c_str());
  key(Key::Confirm);require(!app.title,"load native fixture from title");
  if(index==0){action(UiAction::ReadEvent);scroll(1);action(UiAction::CloseEventReader);key(Key::Save);}
  else if(index==1){key(Key::Detail);key(Key::Down);key(Key::Back);key(Key::Right);key(Key::Save);key(Key::Load);}
  else if(index==2||index==3){
   const int turn=app.game.turn;key(Key::EndTurn);require(app.game.turn==turn+1,"original enemy move executes through normal End Turn");
   for(unsigned i=0;i<40;i++)advance(250);
   key(Key::Statuses);key(Key::Back);key(Key::Save);key(Key::Load);advance(250);
  }else if(index==4){
   action(UiAction::ReadEvent);scroll(1);action(UiAction::CloseEventReader);click(HitKind::Choice,0);key(Key::Save);click(HitKind::Choice,6);key(Key::Save);
  }else{
   click(HitKind::Choice,1);require(app.game.phase==Phase::OriginalEventChoice,"Crystal enters its board choice");
   click(HitKind::CrystalCell,0);action(UiAction::CrystalLarge);click(HitKind::CrystalCell,60);key(Key::Save);key(Key::Load);
  }
  require(saved>0,"case exercises production save callback");std::fprintf(manifest,"],\"saves\":%u}",saved);total_frames+=frames;total_saves+=saved;
 }
 std::fputs("]}\n",manifest);require(!std::fclose(manifest),"close manifest");std::printf("Original-world backend fixtures: 6 cases, %u frames, %u saves; no original-PC differential claim\n",total_frames,total_saves);
}
