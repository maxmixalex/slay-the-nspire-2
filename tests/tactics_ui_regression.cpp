#include "app.hpp"
#include "card_grid_fixture.hpp"
#include "content_fixture.hpp"
#include "status.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static App a;static SaveData stored;static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
static bool save_cb(const State&s){return save(s,stored);}
static bool load_cb(State&s){return load(s,stored.bytes,stored.size);}
static SaveData snapshot(){SaveData b{};CHECK(save(a.game,b));return b;}
static void unchanged(const SaveData&b){auto now=snapshot();CHECK(now.size==b.size&&!memcmp(now.bytes,b.bytes,b.size));}
static bool click(HitKind kind,int value){a.render();for(int i=0;i<a.hit_count;i++){auto h=a.hits[i];if(h.kind==kind&&h.value==value){a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);return true;}}return false;}
static void action(UiAction action){CHECK(click(HitKind::UiAction,(int)action));}
static void setup(){
 new_run(a.game,7,false);auto&s=a.game;s.phase=Phase::Combat;s.floor=s.turn=s.enemy_count=1;s.energy=9;
 s.current_node=0;s.deck_count=s.card_count=s.hand.count=1;s.cards[0]=s.deck[0]={Headbutt,0};s.hand.ids[0]=0;s.zones[0]=Zone::Hand;s.enemies[0]={46,46,0,0,0,0};
 for(int i=1;i<=27;i++){s.cards[i]={Defend,0};s.zones[i]=Zone::Discard;s.discard.ids[s.discard.count++]=(uint16_t)i;++s.card_count;}
 a.reset_presentation();a.title=a.library=a.paused=a.help=false;a.callbacks={save_cb,load_cb};CHECK(validate(s));
}
static void return_choice(){
 setup();a.input(Key::Confirm);a.input(Key::Confirm);CHECK(a.game.phase==Phase::ReturnChoice&&a.game.hand.count==0);auto before=snapshot();
 a.render();for(int i=0;i<a.hit_count;i++)CHECK(a.hits[i].kind!=HitKind::Hand&&a.hits[i].kind!=HitKind::Enemy);
 for(int row=0;row<5;++row)a.input(Key::Down);CHECK(a.cursor==20);CHECK(spire_test::settle_card_grid(a));unchanged(before);
 CHECK(click(HitKind::CombatChoice,22));CHECK(a.choice_selected_count==1&&a.choice_selected[0]==22);unchanged(before);
 a.input(Key::Detail);CHECK(a.detail_card==Defend);a.input(Key::Back);CHECK(a.choice_selected_count==1&&a.game.phase==Phase::ReturnChoice);unchanged(before);
 a.input(Key::Save);CHECK(stored.size);a.pointer_scroll(-9);CHECK(spire_test::settle_card_grid(a));CHECK(a.choice_selected[0]==22);unchanged(before);
 action(UiAction::ConfirmChoice);CHECK(a.game.phase==Phase::Combat&&a.game.draw.ids[a.game.draw.count-1]==22);
 a.input(Key::Load);CHECK(a.game.phase==Phase::ReturnChoice&&a.choice_selected_count==0);unchanged(before);
 // Capturing a card, then scrolling, must never select the replacement region.
 a.render();a.pointer_move(35,70);a.pointer_button(true);a.pointer_scroll(1);a.pointer_button(false);CHECK(a.choice_selected_count==0);unchanged(before);
 a.cursor=25;a.input(Key::Confirm);CHECK(a.choice_selected_count==1&&a.choice_selected[0]==26);a.input(Key::EndTurn);CHECK(a.game.phase==Phase::Combat&&a.game.draw.ids[a.game.draw.count-1]==26);
}
static void statuses(){
 setup();auto&s=a.game;s.strength=2;s.dexterity=3;s.vigor=8;s.thorns=3;s.pain=3;s.embrace=1;s.power_order[0]=0;s.power_order[1]=1;s.power_count=2;
 s.no_draw=1;s.no_block=2;s.noxious_fumes=2;s.accuracy=4;s.demon_form=3;s.infinite_blades=1;s.flame_barrier=4;s.block_next_turn=9;s.draw_next_turn=2;
 s.enemies[0].strength=4;s.enemies[0].weak=2;s.enemies[0].vulnerable=1;s.enemies[0].poison=7;
 auto before=snapshot();int ids[StatusKinds];CHECK(collect_statuses(s,0,ids)==15);CHECK(collect_statuses(s,1,nullptr)==4);
 CHECK(click(HitKind::StatusActor,0));CHECK(a.status_actor==0);unchanged(before);
 for(int i=0;i<15;i++){
  a.status_selected=i;a.render();CHECK(click(HitKind::StatusItem,i));
  for(int j=0;j<a.hit_count;j++)CHECK(a.hits[j].kind==HitKind::StatusActor||a.hits[j].kind==HitKind::StatusItem||a.hits[j].kind==HitKind::Close||a.hits[j].kind==HitKind::Scroll);
  a.input(Key::EndTurn);a.input(Key::Collection);a.input(Key::ReferenceMap);CHECK(a.status_actor==0&&!a.library&&!a.map_preview.open);unchanged(before);
 }
 a.status_selected=0;CHECK(click(HitKind::Scroll,6));CHECK(a.status_selected==6);unchanged(before);
 CHECK(click(HitKind::Scroll,6));CHECK(a.status_selected==12);unchanged(before);
 CHECK(click(HitKind::Scroll,-6));CHECK(a.status_selected==6);unchanged(before);
 a.input(Key::Right);CHECK(a.status_actor==1&&a.status_selected==0);a.input(Key::Down);CHECK(a.status_selected==1);unchanged(before);
 CHECK(click(HitKind::StatusActor,0));CHECK(a.status_selected==0);a.pointer_scroll(1);CHECK(a.status_selected==1);unchanged(before);
 a.input(Key::Statuses);CHECK(a.status_actor<0);a.input(Key::Statuses);CHECK(a.status_actor==0);a.input(Key::Back);CHECK(a.status_actor<0);unchanged(before);
 // Status inspection can temporarily cover a pending choice, without completing it.
 CHECK(play(s,0,0));a.reset_presentation();CHECK(s.phase==Phase::ReturnChoice);before=snapshot();
 a.input(Key::Statuses);CHECK(a.status_actor==0);a.input(Key::EndTurn);unchanged(before);a.input(Key::Back);CHECK(s.phase==Phase::ReturnChoice);unchanged(before);
}
int main(int argc,char**argv){
 std::vector<uint8_t> data;
 if(argc>1){FILE*f=fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));size_t n=fread(data.data(),1,data.size(),f);fclose(f);CHECK(a.init(data.data(),n,{save_cb,load_cb}));}
 return_choice();statuses();
 if(!data.empty()){
  CHECK(a.content.card_count()==CardKinds);for(int id=Headbutt;id<CardKinds;id++)CHECK(a.content.image(a.content.card_image(id)).pixels);
  for(int id=U_FeelNoPain;id<U_Count;id++)CHECK(a.content.image(a.content.ui_image(id)).pixels);
  CHECK(strstr(a.content.card_string(Backstab,1),"固有"));CHECK(strstr(a.content.card_string(InfiniteBlades,2),"固有"));
  CHECK(!strstr(a.content.card_string(InfiniteBlades,1),"固有"));
  a.library=true;char text[512];for(int id=Headbutt;id<CardKinds;id++)for(int up=0;up<2;up++){a.card_description({(CardKind)id,(uint8_t)up},0,text,sizeof text);CHECK(*text&&!strchr(text,'{'));}
 }
 printf("tactics UI: %u checks passed\n",checks);
}
