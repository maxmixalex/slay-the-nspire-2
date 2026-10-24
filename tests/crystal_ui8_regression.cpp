#include "app.hpp"
#include "original_events.hpp"
#include "world_run.hpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App app;static SaveData before,after;static unsigned checks=0;static const char*captures=nullptr;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"crystal UI8 line %d: %s phase=%d\n",__LINE__,#x,int(app.game.phase));std::exit(1);}}while(0)
static void capture(const char*name){
 if(!captures)return;app.render();const std::string path=std::string(captures)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);
 std::fprintf(f,"P6\n320 240\n255\n");for(auto p:app.pixels){const unsigned char rgb[]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}CHECK(!std::fclose(f));
}
static unsigned hits(){unsigned n=0;for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::CrystalCell){const auto&h=app.hits[i];CHECK(crystal_layout::exists(h.value%11,h.value/11));CHECK(h.x>=61&&h.x+h.w<=185&&h.y>=77&&h.y+h.h<=201);++n;}return n;}
int main(int argc,char**argv){
 std::vector<uint8_t>pack;
 if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);pack.resize(spire_test::pack_size(f));CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());std::fclose(f);CHECK(app.content.open(pack.data(),pack.size())&&app.content.current_profile());}
 if(argc>2)captures=argv[2];
 new_run(app.game,7319,false,Character::Ironclad);CHECK(world_begin_run(app.game));app.game.original_events={};
 // Isolate this event within the already-open event point. Completing Neow
 // first and then reopening the same point manufactured two finished rooms
 // for one visited floor, which correctly cannot be serialized.
 CHECK(app.game.world.room_open==1&&app.game.world.rooms_completed==0);
 CHECK(original_event_begin(app.game,OriginalEventId::CrystalSphere,31,1));app.game.gold=250;CHECK(original_event_choose(app.game,1));
 app.title=false;app.reset_presentation();CHECK(save(app.game,before));app.render();CHECK(!hits());
 const auto start=app.crystal_presentation.start;app.input(Key::Confirm);CHECK(app.game.original_events.crystal_charges==6);
 for(unsigned i=0;i<16;++i)app.advance(33);app.render();CHECK(hits()==97);CHECK(app.crystal_presentation.start==start);
 CHECK(app.cursor==60);capture("crystal-entry");
 app.input(Key::DiscardPile);CHECK(app.game.original_events.crystal_tool==2);app.render();
 app.advance(150);app.render();CHECK(app.crystal_presentation.hover_alpha(60,app.map_clock_ms)==128);CHECK(app.crystal_presentation.hover_alpha(48,app.map_clock_ms)==128);
 app.input(Key::DrawPile);CHECK(app.game.original_events.crystal_tool==1);
 // Navigation and both original hotkeys affect selection only; no acquisition
 // or reward reroll can occur while painting or advancing the presentation.
 CHECK(save(app.game,before));for(unsigned i=0;i<20;++i){app.advance(33);app.render();}
 CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
 for(unsigned turn=0;turn<6;++turn){
  unsigned cell=0;while(cell<CrystalCells&&app.game.original_events.crystal_clear[cell])++cell;CHECK(cell<CrystalCells);
  app.cursor=int(cell);app.render();const int x=crystal_layout::cell_edge(cell%11)+5,y=crystal_layout::cell_top(cell/11)+5;
  app.pointer_move(x,y);app.pointer_button(true);app.pointer_button(false);
  CHECK(app.game.original_events.crystal_charges==5-turn);CHECK(app.game.original_events.crystal_clear[cell]);
  CHECK(app.crystal_presentation.cell_alpha(cell,app.map_clock_ms)==255);
  app.advance(250);app.render();CHECK(app.crystal_presentation.cell_alpha(cell,app.map_clock_ms)>=127&&app.crystal_presentation.cell_alpha(cell,app.map_clock_ms)<=128);
  if(!turn)capture("crystal-reveal-half");app.advance(250);app.render();CHECK(app.crystal_presentation.cell_alpha(cell,app.map_clock_ms)==0);
  if(turn<5)CHECK(!app.game.original_events.crystal_clear[app.cursor]);
 }
 CHECK(app.game.original_events.reward_active);capture("crystal-rewards");CHECK(original_event_reward(app.game,-1));
 CHECK(app.game.original_events.finished);app.render();capture("crystal-finished");
 bool proceed=false;for(int i=0;i<app.hit_count;++i){CHECK(app.hits[i].kind!=HitKind::CrystalCell);if(app.hits[i].kind==HitKind::Choice&&app.hits[i].value==0)proceed=true;}
 CHECK(proceed);CHECK(save(app.game,before));for(unsigned i=0;i<120;++i){app.advance(33);app.render();}
 CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
 const auto completed=app.game.world.rooms_completed;CHECK(!app.map_window.is_open);
 app.input(Key::Confirm);CHECK(app.game.phase==Phase::Event&&app.game.original_events.id==OriginalEventId::CrystalSphere);
 CHECK(app.game.world.room_open==2&&app.game.world.rooms_completed==completed+1&&world_can_travel(app.game)&&app.map_window.is_open);
 // Proceed opens the map over the retained event; closing and reopening it
 // must not replay the minigame or count the room a second time.
 CHECK(save(app.game,before));app.input(Key::Back);CHECK(!app.map_window.is_open);
 app.input(Key::Confirm);CHECK(app.map_window.is_open&&app.game.world.rooms_completed==completed+1);
 CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
 // Banner remains keyed to the room, not to a changing page/reward revision.
 AncientBannerPresentation banner;banner.observe(app.game,100);const auto begun=banner.start;app.game.original_events.page++;banner.observe(app.game,900);CHECK(banner.start==begun);
 CHECK(banner.alpha(4600)==255);CHECK(banner.alpha(5100)<64);CHECK(banner.epithet_alpha(1600)==0);CHECK(banner.epithet_alpha(7600)==128);
 CHECK(AncientBannerPresentation::spring(0)==0&&AncientBannerPresentation::spring(3000)==4096);
 CHECK(AncientBannerPresentation::circ(2000,4000)>3500&&AncientBannerPresentation::circ(4000,4000)==4096);
 std::printf("Crystal UI8: %u checks; source grid/tools, hover/reveal, real pointer acquisition, terminal screen and presentation purity passed.\n",checks);
}
