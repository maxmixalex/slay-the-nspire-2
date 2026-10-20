#include "app.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App a;static SaveData stored;static unsigned checks=0;static std::string capture_dir;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
static bool save_cb(const State&s){return save(s,stored);}
static bool load_cb(State&s){return load(s,stored.bytes,stored.size);}
static SaveData snapshot(){SaveData b{};CHECK(save(a.game,b));return b;}
static void same(const SaveData&b){auto now=snapshot();CHECK(now.size==b.size&&!memcmp(now.bytes,b.bytes,b.size));}
static bool click(HitKind kind,int value){a.render();for(int i=0;i<a.hit_count;i++){auto h=a.hits[i];if(h.kind==kind&&h.value==value){a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);return true;}}return false;}
static void shot(const char*name){if(capture_dir.empty())return;a.pointer.visible=false;a.render();std::string path=capture_dir+"/"+name+".ppm";FILE*f=fopen(path.c_str(),"wb");CHECK(f);fprintf(f,"P6\n320 240\n255\n");for(auto p:a.pixels){unsigned char v[3]={(unsigned char)((p>>11)*255/31),(unsigned char)(((p>>5)&63)*255/63),(unsigned char)((p&31)*255/31)};CHECK(fwrite(v,1,3,f)==3);}CHECK(!fclose(f));}
static void setup(){a.title=a.library=a.paused=a.help=false;a.map_preview.open=false;a.character_select=false;new_run(a.game,91,true);for(const auto&d:relic_definitions)gain_relic(a.game,d.id);a.reset_presentation();a.callbacks={save_cb,load_cb};}
static void inventory(){
 setup();for(int i=25;i<=28;i++)a.game.relic_counters[i]=2;a.game.relic_counters[29]=9;
 auto before=snapshot();a.render();
 // Each HUD icon addresses its own owned instance, not the first relic.
 for(int i=0;i<15;i++){
  CHECK(click(HitKind::OwnedRelic,i));CHECK(a.detail_item==a.game.relic_order[i]&&a.detail_owned==i);same(before);
  a.input(Key::EndTurn);a.input(Key::Statuses);same(before);CHECK(a.status_actor<0);
  a.input(Key::Back);CHECK(a.detail_item<0&&a.detail_owned<0);
 }
 a.input(Key::Relics);CHECK(a.panel==5);a.pointer_scroll(18);a.render();CHECK(a.scroll==18);shot("relic-grid");
 for(int i=18;i<30;i++){
  CHECK(click(HitKind::Relic,i));CHECK(a.detail_owned==i);same(before);a.input(Key::Back);CHECK(a.panel==5);
 }
 a.scroll=26;a.input(Key::Detail);CHECK(a.detail_item==26); // Model index, not relic mask.
}
static void treasure(){
 setup();a.game.phase=Phase::Treasure;a.game.current_node=19;a.game.next_row=6;a.game.floor=7;
 // Start with only the base relic, so an actual unowned reward can be revealed.
 new_run(a.game,91,false);a.game.phase=Phase::Treasure;a.game.current_node=19;a.game.next_row=6;a.game.floor=7;a.reset_presentation();
 auto closed=snapshot();a.render();same(closed);CHECK(click(HitKind::Choice,0));CHECK(a.game.treasure_state==1);auto revealed=snapshot();shot("treasure-reveal");
 CHECK(a.game.relic_count==1);a.input(Key::Detail);CHECK(a.detail_item>=0);a.input(Key::Back);same(revealed);
 a.input(Key::Save);CHECK(click(HitKind::Choice,0));CHECK(a.game.treasure_state==2&&a.game.relic_count==2);auto claimed=snapshot();
 CHECK(click(HitKind::Choice,0));same(claimed);a.input(Key::Load);CHECK(a.game.treasure_state==1&&a.game.relic_count==1);same(revealed);
 // A capture started before load cannot claim the reward when released.
 a.render();a.pointer_move(150,110);a.pointer_button(true);a.input(Key::Load);a.pointer_button(false);same(revealed);
 CHECK(click(HitKind::Choice,1));CHECK(a.game.phase==Phase::Map&&a.game.relic_count==1);
 CHECK(load(a.game,revealed.bytes,revealed.size));a.reset_presentation();CHECK(click(HitKind::Choice,0));a.input(Key::Save);CHECK(click(HitKind::Choice,1));a.input(Key::Load);
 CHECK(a.game.phase==Phase::Treasure&&a.game.treasure_state==2&&a.game.relic_count==2);a.cursor=1;a.input(Key::Confirm);CHECK(a.game.phase==Phase::Map);
}
int main(int argc,char**argv){
 std::vector<uint8_t> data;
 if(argc>1){FILE*f=fopen(argv[1],"rb");CHECK(f);data.resize(MaxPack);size_t n=fread(data.data(),1,data.size(),f);fclose(f);CHECK(a.init(data.data(),n,{save_cb,load_cb}));}
 if(argc>2)capture_dir=argv[2];inventory();a.input(Key::Back);a.input(Key::Back);
 setup();auto before=snapshot();a.pointer_move(25,29);a.render();for(int i=0;i<60;i++){a.advance(33);a.render();}same(before);
 unsigned strike=0;while(strike<a.game.hand.count&&a.game.cards[a.game.hand.ids[strike]].kind!=Strike)++strike;CHECK(strike<a.game.hand.count);
 a.game.relic_counters[25]=a.game.relic_counters[26]=a.game.relic_counters[27]=2;a.game.relic_counters[29]=9;
 CHECK(play(a.game,int(strike),0));a.render();CHECK(a.relic_flash_ticks==30&&(a.relic_flash_mask&Shuriken));before=snapshot();
 for(int i=0;i<31;i++)a.tick();a.render();CHECK(!a.relic_flash_ticks);same(before);a.open_owned_relic(26);shot("relic-detail");a.input(Key::Back);
 treasure();printf("relic UI: %u checks passed\n",checks);
}
