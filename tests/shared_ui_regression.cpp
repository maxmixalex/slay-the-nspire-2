#include "app.hpp"
#include "card_grid_fixture.hpp"
#include "content_fixture.hpp"
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
static void settle_deck_controls(){
    const auto before=snapshot();a.render();a.advance(250);a.advance(100);a.render();unchanged(before);
    // The native Confirm starts outside the viewport and enters in350ms.
}
static void confirm_deck(){settle_deck_controls();action(UiAction::ConfirmDeckChoice);}
static void init(){new_run(a.game,7,true);a.reset_presentation();a.title=a.library=a.paused=a.help=a.compendium_open=a.statistics_open=a.history_open=false;}
int main(int argc,char**argv){
 std::vector<uint8_t> data;
 if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));size_t n=std::fread(data.data(),1,data.size(),f);std::fclose(f);CHECK(a.init(data.data(),n,{}));}
 init();auto before=snapshot();a.input(Key::Collection);action(UiAction::LibraryRelics);a.render();
 // Categories occupy one scroll container; relic clicks open the original
 // NInspectRelicScreen and return to the same collection scroll position.
 CHECK(a.collection.count>RelicKinds);unsigned count=0;for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==HitKind::LibraryItem){++count;CHECK(a.hits[i].y>=0&&a.hits[i].y+a.hits[i].h<=240);}CHECK(count>0);unchanged(before);
 a.library_selected=24;CHECK(spire_test::focus_card_grid(a));CHECK(click(HitKind::LibraryItem,24));CHECK(a.library_selected==24&&a.detail_item==a.collection.ids[24]);unchanged(before);a.input(Key::Back);CHECK(a.detail_item<0&&a.library);a.advance(250);
 CHECK(spire_test::drag_grid_thumb(a,true));CHECK(spire_test::drag_grid_thumb(a,false));unchanged(before);
 a.input(Key::Back);CHECK(!a.library&&a.compendium_open);a.input(Key::Back);CHECK(!a.compendium_open);
 init();a.game.phase=Phase::Upgrade;a.cursor=31;CHECK(spire_test::focus_card_grid(a));before=snapshot();CHECK(click(HitKind::Choice,31));CHECK(a.deck_candidate==31);unchanged(before);
 a.render();for(int i=0;i<a.hit_count;i++)CHECK(a.hits[i].kind!=HitKind::Choice&&a.hits[i].kind!=HitKind::Enemy);
 a.input(Key::Back);CHECK(a.deck_candidate<0&&a.game.phase==Phase::Upgrade);unchanged(before);
 a.input(Key::Confirm);CHECK(a.deck_candidate==31);confirm_deck();CHECK(a.game.deck[31].upgraded&&a.game.phase==Phase::Complete);
 init();a.game.practice=0;a.game.phase=Phase::Remove;a.game.gold=999;a.cursor=2;int gold=a.game.gold,deck=a.game.deck_count;before=snapshot();a.input(Key::Confirm);CHECK(a.deck_candidate==2);unchanged(before);
 settle_deck_controls();action(UiAction::Back);CHECK(a.deck_candidate<0&&a.game.phase==Phase::Remove);unchanged(before);a.input(Key::Confirm);confirm_deck();CHECK(a.game.deck_count==deck-1&&a.game.gold==gold-75&&a.game.phase==Phase::Shop);
 init();a.game.phase=Phase::Transform;a.game.event_id=EventId::Grove;a.game.event_remaining=2;a.cursor=3;before=snapshot();a.input(Key::Confirm);unchanged(before);confirm_deck();CHECK(a.game.event_remaining==1&&a.game.event_first==3&&a.game.phase==Phase::Transform);
 before=snapshot();a.cursor=3;CHECK(spire_test::focus_card_grid(a));CHECK(a.cursor!=3&&a.deck_candidate<0);
 for(int i=0;i<a.hit_count;++i)CHECK(a.hits[i].kind!=HitKind::Choice||a.hits[i].value!=3);unchanged(before);
 a.cursor=4;a.input(Key::Confirm);CHECK(a.deck_candidate==4);a.game.revision++;auto stale=snapshot();a.confirm_deck_choice();unchanged(stale);CHECK(a.deck_candidate<0);
 init();a.game.cards[a.game.hand.ids[0]]={Purity,0};a.game.event_cards.deck_origin[a.game.hand.ids[0]]=0;a.cursor=0;a.input(Key::Confirm);a.input(Key::Confirm);CHECK(a.game.phase==Phase::ExhaustManyChoice);before=snapshot();a.render();
 for(int i=0;i<a.hit_count;i++)CHECK(a.hits[i].kind!=HitKind::Hand&&a.hits[i].kind!=HitKind::Enemy);unchanged(before);a.advance(250);a.advance(100);action(UiAction::ConfirmChoice);CHECK(a.game.phase==Phase::Combat&&a.game.exhaust.count==1);
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
