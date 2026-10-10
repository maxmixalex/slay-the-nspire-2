#include "app.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static unsigned checks=0;static App a;static SaveData stored{};
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
static bool save_cb(const State&s){return save(s,stored);}
static bool load_cb(State&s){return load(s,stored.bytes,stored.size);}
static bool click(HitKind kind,int value){a.render();for(int i=0;i<a.hit_count;i++){auto h=a.hits[i];if(h.kind==kind&&h.value==value){a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);return true;}}return false;}
static void action(UiAction x){CHECK(click(HitKind::UiAction,(int)x));}
static void same(const SaveData&b){SaveData now{};CHECK(save(a.game,now));CHECK(now.size==b.size&&!memcmp(now.bytes,b.bytes,b.size));}
int main(int argc,char**argv){
 std::vector<uint8_t> content;
 if(argc>1){FILE*f=fopen(argv[1],"rb");CHECK(f);content.resize(MaxPack);size_t n=fread(content.data(),1,content.size(),f);fclose(f);CHECK(a.init(content.data(),n,{save_cb,load_cb}));}
 else a.callbacks={save_cb,load_cb};
 a.input(Key::Confirm);CHECK(a.character_select);action(UiAction::SelectSilent);CHECK(a.selected_character==Character::Silent);action(UiAction::StartCharacter);
 CHECK(!a.title&&a.game.character==Character::Silent&&a.game.hp==70&&a.game.deck_count==12);CHECK(choose_node(a.game,0));CHECK(a.game.hand.count==7);
 SaveData prior{};CHECK(save(a.game,prior));a.input(Key::Collection);
 const unsigned counts[]={68,29,30,9};
 for(unsigned filter=0;filter<4;filter++){
  action((UiAction)((int)UiAction::FilterAll+filter));CHECK(a.library_items()==counts[filter]);uint8_t ids[CardKinds];a.library_items(ids);
  for(unsigned i=0;i<counts[filter];i++){
   a.library_page=i/8;a.library_selected=i;CHECK(click(HitKind::LibraryItem,i));CHECK((a.detail_card&255)==ids[i]);a.input(Key::Back);same(prior);
  }
  action(UiAction::CycleSort);CHECK(a.library_sort==1);a.library_items(ids);
  for(unsigned i=1;i<counts[filter];i++)CHECK(cost({ids[i-1],0})<=cost({ids[i],0}));
  action(UiAction::CycleSort);CHECK(a.library_sort==2);a.library_items(ids);
  for(unsigned i=1;i<counts[filter];i++)CHECK(definitions[ids[i-1]].type<=definitions[ids[i]].type);
  action(UiAction::CycleSort);CHECK(a.library_sort==0);same(prior);
 }
 action(UiAction::LibraryRelics);CHECK(click(HitKind::LibraryItem,16));CHECK(a.library_selected==16);CHECK(a.detail_item==16);a.input(Key::Back);CHECK(a.library);a.input(Key::Back);CHECK(!a.library);
 // Construct a choice using real instance IDs; production UI must stay modal.
 new_run(a.game,7,true,Character::Silent);a.reset_presentation();a.title=false;
 a.game.cards[a.game.hand.ids[1]]={Prepared,1};a.cursor=1;a.input(Key::Confirm);
 CHECK(a.game.phase==Phase::DiscardChoice&&a.game.discard_required==2);a.busy=0;a.render();CHECK(save(a.game,prior));
 for(int i=0;i<a.hit_count;i++)CHECK(a.hits[i].kind!=HitKind::Enemy&&a.hits[i].kind!=HitKind::Hand);
 int first=a.game.hand.ids[0],second=a.game.hand.ids[1];
 CHECK(click(HitKind::CombatChoice,first));CHECK(a.choice_selected_count==1);same(prior);
 action(UiAction::ConfirmChoice);CHECK(a.game.phase==Phase::DiscardChoice);same(prior);
 CHECK(click(HitKind::CombatChoice,second));CHECK(a.choice_selected_count==2);same(prior);
 a.input(Key::Detail);CHECK(a.detail_card>=0);a.input(Key::Back);CHECK(a.game.phase==Phase::DiscardChoice&&a.choice_selected_count==2);
 // Save preserves the mandatory game choice, not a partially selected UI gesture.
 a.input(Key::Save);CHECK(stored.size>0);action(UiAction::ConfirmChoice);CHECK(a.game.phase==Phase::Combat);
 CHECK(a.game.zones[first]==Zone::Discard&&a.game.zones[second]==Zone::Discard);a.input(Key::Load);a.render();
 CHECK(a.game.phase==Phase::DiscardChoice&&a.choice_selected_count==0&&a.game.discard_required==2);same(prior);
 a.cursor=0;a.input(Key::Confirm);a.input(Key::Right);a.input(Key::Confirm);a.input(Key::EndTurn);CHECK(a.game.phase==Phase::Combat);
 // Exhaust uses the same separate grid and confirmation; it never ends the turn.
 new_run(a.game,7,true,Character::Ironclad);a.reset_presentation();a.game.energy=9;
 a.cursor=2;a.input(Key::Confirm);CHECK(a.game.phase==Phase::ExhaustChoice);a.busy=0;int turn=a.game.turn;
 a.input(Key::Confirm);CHECK(a.game.phase==Phase::ExhaustChoice);a.input(Key::EndTurn);CHECK(a.game.phase==Phase::Combat&&a.game.turn==turn);
 printf("Silent UI: %u checks passed\n",checks);
}
