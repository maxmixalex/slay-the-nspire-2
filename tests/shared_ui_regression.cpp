#include "app.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static App a;static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static bool click(HitKind kind,int value){a.render();for(int i=0;i<a.hit_count;i++){auto h=a.hits[i];if(h.kind==kind&&h.value==value){a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);return true;}}return false;}
static void action(UiAction u){CHECK(click(HitKind::UiAction,(int)u));}
static SaveData snapshot(){SaveData b{};CHECK(save(a.game,b));return b;}
static void unchanged(const SaveData&b){auto now=snapshot();CHECK(b.size==now.size&&!std::memcmp(b.bytes,now.bytes,b.size));}
static void init(){new_run(a.game,7,true);a.reset_presentation();a.title=a.library=a.paused=a.help=false;}
int main(int argc,char**argv){
 std::vector<uint8_t> data;
 if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(MaxPack);size_t n=std::fread(data.data(),1,data.size(),f);std::fclose(f);CHECK(a.init(data.data(),n,{}));}
 init();auto before=snapshot();a.input(Key::Collection);action(UiAction::LibraryRelics);action(UiAction::NextPage);a.render();CHECK(a.library_page==1);
 // The three appended starter relics fill a third row on this page. All
 // item targets must still end before the selected name at y=163.
 unsigned count=0;for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==HitKind::LibraryItem){++count;CHECK(a.hits[i].value>=18&&a.hits[i].value<RelicKinds);CHECK(a.hits[i].y+a.hits[i].h<=160);}CHECK(count==RelicKinds-18);unchanged(before);
 CHECK(click(HitKind::LibraryItem,24));CHECK(a.detail_item==24);a.render();for(int i=0;i<a.hit_count;i++)CHECK(a.hits[i].kind!=HitKind::LibraryItem);unchanged(before);
 a.input(Key::Back);CHECK(a.library&&a.library_page==1);a.input(Key::Back);CHECK(!a.library);
 init();a.game.phase=Phase::Upgrade;a.cursor=31;before=snapshot();CHECK(click(HitKind::Choice,31));CHECK(a.deck_candidate==31);unchanged(before);
 a.render();for(int i=0;i<a.hit_count;i++)CHECK(a.hits[i].kind!=HitKind::Choice&&a.hits[i].kind!=HitKind::Enemy);
 a.input(Key::Back);CHECK(a.deck_candidate<0&&a.game.phase==Phase::Upgrade);unchanged(before);
 a.input(Key::Confirm);CHECK(a.deck_candidate==31);action(UiAction::ConfirmDeckChoice);CHECK(a.game.deck[31].upgraded&&a.game.phase==Phase::Complete);
 init();a.game.practice=0;a.game.phase=Phase::Remove;a.game.gold=999;a.cursor=2;int gold=a.game.gold,deck=a.game.deck_count;before=snapshot();a.input(Key::Confirm);CHECK(a.deck_candidate==2);unchanged(before);
 action(UiAction::CancelDeckChoice);unchanged(before);a.input(Key::Confirm);action(UiAction::ConfirmDeckChoice);CHECK(a.game.deck_count==deck-1&&a.game.gold==gold-75&&a.game.phase==Phase::Shop);
 init();a.game.phase=Phase::Transform;a.game.event_id=EventId::Grove;a.game.event_remaining=2;a.cursor=3;before=snapshot();a.input(Key::Confirm);unchanged(before);action(UiAction::ConfirmDeckChoice);CHECK(a.game.event_remaining==1&&a.game.event_first==3&&a.game.phase==Phase::Transform);
 a.cursor=3;a.input(Key::Confirm);CHECK(a.deck_candidate<0);a.cursor=4;a.input(Key::Confirm);CHECK(a.deck_candidate==4);a.game.revision++;auto stale=snapshot();a.confirm_deck_choice();unchanged(stale);CHECK(a.deck_candidate<0);
 init();a.game.cards[a.game.hand.ids[0]]={Purity,0};a.cursor=0;a.input(Key::Confirm);CHECK(a.game.phase==Phase::ExhaustManyChoice);before=snapshot();a.render();
 for(int i=0;i<a.hit_count;i++)CHECK(a.hits[i].kind!=HitKind::Hand&&a.hits[i].kind!=HitKind::Enemy);unchanged(before);action(UiAction::ConfirmChoice);CHECK(a.game.phase==Phase::Combat&&a.game.exhaust.count==1);
 if(!data.empty()){
  CHECK(a.content.frame_image({StrikeSilent,0})==a.content.ui_image(U_GreenAttack));CHECK(a.content.energy_image({StrikeSilent,0})==a.content.ui_image(U_SilentEnergy));
  for(int i=17;i<30;i++)CHECK(a.content.relic_image(i)==a.content.ui_image(i>=25?U_Kunai+i-25:U_HornCleat+i-17));
  const int starters[]={U_CrackedCore,U_DivineRight,U_BoundPhylactery};
  for(int i=0;i<3;i++)CHECK(a.content.relic_image(30+i)==a.content.ui_image(starters[i]));
  CHECK(std::strstr(a.content.card_string(Scrawl,2),"保留"));CHECK(!std::strstr(a.content.card_string(Scrawl,1),"保留"));
  CHECK(std::strstr(a.content.card_string(Purity,1),"保留"));
  char desc[512];a.card_description({HandOfGreed,1},0,desc,sizeof desc);CHECK(!std::strchr(desc,'{'));CHECK(std::strstr(desc,"25"));
 }
 std::printf("shared UI: %u checks passed\n",checks);
}
