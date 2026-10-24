#include "app.hpp"
#include "ui_core_observer.hpp"
#include "world_run.hpp"
#include "run_options.hpp"
#include "original_events.hpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static App a;
static UiProgress restored,fixture;
static UiHistoryJournal ring;
static uint8_t bytes[UiProgressCapacity],backup[UiProgressCapacity];
static SaveData before,after;
static unsigned checks,load_mode;
static size_t bad_size,backup_size;
static const char*scenario="initial";
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"history journal:%d [%s] %s phase=%u\n",__LINE__,scenario,#x,unsigned(a.game.phase));std::exit(1);}}while(0)
static unsigned amount(RunHistoryEvent kind){unsigned total=0;const int owner=a.progress.journal.owner(a.progress.current.serial);for(unsigned i=0;i<a.progress.journal.count;++i){const auto&e=a.progress.journal.at(i);if(e.owner==owner&&e.kind==kind)total+=kind>=RunHistoryEvent::GoldSpent&&kind<=RunHistoryEvent::MaxHpLost?e.amount():1;}return total;}
static const UiHistoryJournalEntry* last(RunHistoryEvent kind){for(unsigned i=a.progress.journal.count;i;--i)if(a.progress.journal.at(i-1).kind==kind)return &a.progress.journal.at(i-1);return nullptr;}
static void start(unsigned seed=0x7195c3){
 a.reset_presentation();a.title=a.paused=a.help=a.library=a.compendium_open=a.history_open=a.settings_open=false;a.frontend_confirm=0;a.panel=0;
 clear_ui_progress(a.progress);a.progress_loaded=true;a.progress_dirty=false;new_run(a.game,seed,false);
 CHECK(configure_run_options(a.game,RunMode::Custom,run_modifier_bit(RunModifier::Flight)));
 CHECK(world_set_seed(a.game,"NSPIRE-WORLD-017")&&world_begin_run(a.game));a.begin_progress();
 CHECK(a.progress.current.floor_count==1&&a.progress.journal.owner(a.progress.current.serial)>=0);
}
static void event(OriginalEventId id){
 // Isolate the real event command at an already observed Event map point.
 // No synthesized cards, rewards or event options are supplied by the test.
 a.game.original_events={};a.game.world.room_type=unsigned(WorldRoom::Event);a.game.world.room_open=1;a.game.phase=Phase::Map;
 UiCoreObserver observer(a);CHECK(original_event_begin(a.game,id,a.game.world.run_seed,0));
}
static void choices(){
 scenario="event variables captured before consumption, spending and forced discard";start();a.game.gold=200;
 CHECK(procure_potion(a.game,Potion::Blood));event(OriginalEventId::RanwidTheElder);
 {UiCoreObserver observer(a);CHECK(original_event_choose(a.game,1));}
 const auto*chosen=last(RunHistoryEvent::EventChoice);CHECK(chosen&&chosen->model==a.content.world_string_id("events","RANWID_THE_ELDER.pages.INITIAL.options.GOLD.title")&&chosen->detail==100&&chosen->flags==unsigned(HistoryChoiceVariable::Gold));
 CHECK(amount(RunHistoryEvent::GoldSpent)==100&&amount(RunHistoryEvent::GoldLost)==0&&a.progress.current.facts.shop_spent==100);
 start();CHECK(procure_potion(a.game,Potion::Blood));event(OriginalEventId::StoneOfAllTime);
 {UiCoreObserver observer(a);CHECK(original_event_choose(a.game,0));}
 CHECK(amount(RunHistoryEvent::PotionDiscarded)==1&&last(RunHistoryEvent::PotionDiscarded)->model==unsigned(Potion::Blood));
 scenario="FakeMerchant real purchase, no invented EventChoice, remaining stocked offers";start();a.game.gold=9999;event(OriginalEventId::FakeMerchant);
 const auto relic=a.game.original_events.offers[0];const unsigned price=a.game.original_events.choice_prices[0];
 {UiCoreObserver observer(a);CHECK(original_event_choose(a.game,0));}
 CHECK(amount(RunHistoryEvent::RelicBought)==1&&last(RunHistoryEvent::RelicBought)->model==relic&&last(RunHistoryEvent::RelicBought)->detail==price);
 CHECK(amount(RunHistoryEvent::GoldSpent)==price&&!amount(RunHistoryEvent::GoldLost)&&!amount(RunHistoryEvent::EventChoice));
 {UiCoreObserver observer(a);CHECK(original_event_choose(a.game,6));}
 CHECK(amount(RunHistoryEvent::RelicSkipped)==5);const unsigned count=a.progress.journal.count;
 {UiCoreObserver observer(a);CHECK(!original_event_choose(a.game,6));}CHECK(a.progress.journal.count==count);
}
static void card_rewards(){
 scenario="real event CardReward cancel, reroll, claim and remaining skips";start();CHECK(gain_world_relic(a.game,WorldRelicDriftwood));event(OriginalEventId::ColorfulPhilosophers);
 {UiCoreObserver observer(a);CHECK(original_event_choose(a.game,0));}
 CHECK(a.game.phase==Phase::OriginalEventChoice&&a.game.original_events.selection==OriginalEventSelection::Rewards);
 {UiCoreObserver observer(a);CHECK(original_event_reward(a.game,0));}
 CHECK(original_event_has_card_reward(a.game));const unsigned offered=a.game.original_events.offer_count;CHECK(offered==3);
 {UiCoreObserver observer(a);CHECK(original_event_select(a.game,nullptr,0));}
 CHECK(!amount(RunHistoryEvent::CardSkipped)&&!amount(RunHistoryEvent::CardPicked)&&!a.game.original_events.rewards[0].claimed);
 {UiCoreObserver observer(a);CHECK(original_event_reward(a.game,0));CHECK(original_event_can_reroll_reward(a.game)&&original_event_reroll_reward(a.game));}
 CHECK(amount(RunHistoryEvent::CardSkipped)==offered);const unsigned n=a.progress.journal.count;
 {UiCoreObserver observer(a);CHECK(!original_event_reroll_reward(a.game));const uint16_t invalid=9999;CHECK(!original_event_select(a.game,&invalid,1));}
 CHECK(a.progress.journal.count==n);const uint16_t picked=0;const auto token=a.game.original_events.offers[0];
 {UiCoreObserver observer(a);CHECK(original_event_select(a.game,&picked,1));}
 CHECK(amount(RunHistoryEvent::CardPicked)==1&&amount(RunHistoryEvent::CardObtained)==1&&last(RunHistoryEvent::CardObtained)->model==token&&amount(RunHistoryEvent::CardSkipped)==5);
 {UiCoreObserver observer(a);CHECK(original_event_reward(a.game,-1));}
 CHECK(amount(RunHistoryEvent::CardSkipped)==11);CHECK(!original_event_reward(a.game,-1));
}
static void finish_fixture(){
 // Only intermediate routing outcomes are supplied. The target merchant,
 // real combat-entry observations, acquisitions and departure remain real.
 if(a.game.original_events.active)a.game.original_events={};
 a.game.world_relics.choice=WorldRelicChoice::None;a.game.world_relics.step_count=a.game.world_relics.step_cursor=0;
 if(world_room_is_combat(a.game))a.game.phase=Phase::Reward;
 if(a.game.phase==Phase::Treasure)a.game.treasure_state=2;
 UiCoreObserver observer(a);CHECK(world_finish_room(a.game));
}
static void shop_and_retained_rooms(){
 scenario="combat to noncombat cannot inherit retained enemy/turn history";start();
 uint8_t ids[reference::Columns]{};CHECK(world_available_points(a.game,ids,reference::Columns));
 {UiCoreObserver observer(a);CHECK(world_choose_point(a.game,ids[0]));}CHECK(world_room_is_combat(a.game));a.game.turn=9;++a.game.revision;a.sync_progress();
 const unsigned combat_floor=a.progress.current.floor_count-1;CHECK(a.progress.current.floors[combat_floor].encounter&&a.progress.current.floors[combat_floor].turns==9);
 // This route intentionally supplies completed intermediate battles. It is
 // partial history, never evidence of a finished simulated full run.
 for(unsigned guard=0;guard<25&&a.game.phase!=Phase::Shop;++guard){
  finish_fixture();const unsigned n=world_available_points(a.game,ids,reference::Columns);CHECK(n);unsigned chosen=ids[0];const auto&m=*world_current_map(a.game);
  for(unsigned i=0;i<n;++i)if(m.points[ids[i]].type==reference::PointType::Shop){chosen=ids[i];break;}
  {UiCoreObserver observer(a);CHECK(world_choose_point(a.game,chosen));}a.sync_progress();
  if(!world_room_is_combat(a.game)){const auto&floor=a.progress.current.floors[a.progress.current.floor_count-1];CHECK(!floor.encounter&&!floor.turns);}
 }
 CHECK(a.game.phase==Phase::Shop);a.progress.current.partial=1;a.progress.current.facts.complete=0;a.game.gold=9999;
 scenario="map history reader owns its wheel while the underlying route stays fixed";
 a.open_map_window(true);for(unsigned i=0;i<20;++i)a.advance(100);a.render();WorldMapLayout layout;CHECK(world_map_layout(a.game,layout));const auto view=a.map_view(layout);bool hovered=false;
 for(unsigned i=a.progress.current.floor_count-1;i;--i){const auto&floor=a.progress.current.floors[i-1];const int x=layout.points[floor.point].x,y=layout.points[floor.point].y+view.origin;if(y<45||y>220)continue;
  a.pointer_move(x,y);a.render();for(int h=0;h<a.hit_count;++h)if(a.hits[h].kind==HitKind::HistoryJournalScroll)hovered=true;if(hovered)break;}
 CHECK(hovered&&a.history_tooltip.source==2);const auto tooltip=a.history_tooltip;const int scroll=a.map_scroll;
 a.pointer_move(tooltip.x+9,tooltip.y+9);a.render();a.pointer_scroll(1);CHECK(a.map_scroll==scroll);
 a.pointer_move(319,239);a.input(Key::Back);for(unsigned i=0;i<4;++i)a.advance(100);CHECK(!a.map_window.is_open&&!a.map_window.visible);
 scenario="retained terminal reward skips commit only at real departure";
 // Use the real merchant as the retained room after its own generated stock
 // is captured. Opening its map must not record that still-purchasable stock.
 const unsigned skipped=amount(RunHistoryEvent::CardSkipped),relic_skipped=amount(RunHistoryEvent::RelicSkipped);
 {UiCoreObserver observer(a);CHECK(world_finish_room(a.game));}
 CHECK(a.game.world.room_open==2&&amount(RunHistoryEvent::CardSkipped)==skipped&&amount(RunHistoryEvent::RelicSkipped)==relic_skipped);
 const unsigned price=world_shop_price(a.game,0);
 {UiCoreObserver observer(a);CHECK(buy(a.game,0));}
 CHECK(amount(RunHistoryEvent::CardBought)==1&&last(RunHistoryEvent::CardBought)->detail==price&&amount(RunHistoryEvent::GoldSpent)==price);
 const unsigned before_remove=amount(RunHistoryEvent::CardRemoved);
 {UiCoreObserver observer(a);CHECK(buy(a.game,ShopRemove));}
 CHECK(amount(RunHistoryEvent::CardRemoved)==before_remove);
 {UiCoreObserver observer(a);CHECK(remove_card(a.game,0));}
 CHECK(amount(RunHistoryEvent::CardRemoved)==before_remove+1);
 CHECK(world_available_points(a.game,ids,reference::Columns));
 {UiCoreObserver observer(a);CHECK(world_choose_point(a.game,ids[0]));}
 CHECK(amount(RunHistoryEvent::CardSkipped)==skipped+ShopCardCount-1&&amount(RunHistoryEvent::RelicSkipped)==relic_skipped+ShopPotionStart-ShopRelicStart);
}
static bool load_profile(UiProgress&value){if(decode_ui_progress(bytes,bad_size,value))return true;return load_mode==2&&decode_ui_progress(backup,backup_size,value);}
static void persistence(){
 scenario="NSH7 used tail, corrupt late fields and application fallback publication";
 const size_t size=encode_ui_progress(a.progress,bytes,sizeof bytes);CHECK(size&&bytes[3]=='7'&&decode_ui_progress(bytes,size,restored));
 CHECK(restored.journal.count==a.progress.journal.count);for(unsigned i=0;i<restored.journal.count;++i)CHECK(!std::memcmp(&restored.journal.at(i),&a.progress.journal.at(i),sizeof(UiHistoryJournalEntry)));
 UiProgressSummary summary;CHECK(decode_ui_progress_summary(bytes,size,summary)&&summary.exists);
 std::memcpy(backup,bytes,size);backup_size=size;bytes[size-1]=255;bad_size=size;
 CHECK(!decode_ui_progress(bytes,size,restored));
 // Profile cards intentionally read only the small statistics prefix; that
 // preview never publishes a history tail as a loaded profile.
 CHECK(decode_ui_progress_summary(bytes,size,summary)&&summary.exists);
 // A private in-place decode may fail after writing its tail. The production
 // restore contract publishes a cleared profile or a completely valid backup.
 CHECK(save(a.game,before));a.callbacks.load_progress=load_profile;a.progress_loaded=false;load_mode=1;a.restore_progress();
 CHECK(a.progress_loaded&&!a.progress.count&&!a.progress.current.serial&&!a.progress.journal.count);
 a.progress_loaded=false;load_mode=2;a.restore_progress();CHECK(a.progress.journal.count&&a.progress.current.serial);
 CHECK(save(a.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));a.callbacks.load_progress=nullptr;
 scenario="old NSH1 through NSH6 contain no invented journal observations";
 for(unsigned version=2;version<=6;++version){
  clear_ui_progress(fixture);fixture.current.serial=1;fixture.current.floor_count=1;
  if(version>=3){fixture.current.mode=unsigned(RunMode::Custom);fixture.current.modifiers=run_modifier_bit(RunModifier::Flight);}
  if(version>=4){fixture.map_drawing.seed=1;fixture.map_drawing.serial=1;fixture.map_drawing.count=1;fixture.map_drawing.points[0].y_flags=0x8000;}
  if(version>=5)fixture.test_subject_kills=1;if(version>=6)fixture.current.facts.version=1;
  size_t n=encode_ui_progress(fixture,bytes,sizeof bytes);CHECK(n&&bytes[3]=='0'+version&&decode_ui_progress(bytes,n,restored));
  CHECK(!restored.journal.count&&restored.journal.owner(1)<0);
  if(version==2){constexpr size_t discoveries=(CardKinds+7)/8+(WorldRelicKinds+7)/8+(PotionKinds+7)/8+(OriginalEventKinds+7)/8;const size_t tail=28+discoveries+CardKinds*4,old=n-4-CardKinds*4;
   std::memmove(bytes+24,bytes+28,discoveries);std::memmove(bytes+24+discoveries,bytes+tail,n-tail);bytes[3]='1';for(unsigned k=0;k<4;++k)bytes[4+k]=uint8_t(old>>(8*k));
   CHECK(decode_ui_progress(bytes,old,restored)&&restored.journal.owner(1)<0&&!restored.journal.count);}
 }
 scenario="legacy live floor continues forward while its earlier observations remain unknown";
 start();a.progress.journal={};CHECK(encode_ui_progress(a.progress,bytes,sizeof bytes)&&bytes[3]=='6');
 {UiCoreObserver observer(a);CHECK(event_add_deck_card(a.game,{Bash,1})>=0);}
 const int owner=a.progress.journal.owner(a.progress.current.serial);CHECK(owner>=0&&a.progress.journal.owners[owner].incomplete(0)&&amount(RunHistoryEvent::CardObtained)==1);
 scenario="batch transform pending survives profile save and completes exactly once";
 const unsigned pending=a.progress.journal.count;notify_history(a.game,RunHistoryEvent::None);
 a.observe_history(a.game,{RunHistoryEvent::CardTransformed,uint16_t(card_token({Strike,1})),uint16_t(card_token({Bash,0})),1,nullptr});
 size_t n=encode_ui_progress(a.progress,bytes,sizeof bytes);CHECK(n&&decode_ui_progress(bytes,n,restored));a.progress=restored;
 a.observe_history(a.game,{RunHistoryEvent::CardTransformed,Strike,uint16_t(card_token({Bash,1})),2,nullptr});
 CHECK(a.progress.journal.count==pending+1&&last(RunHistoryEvent::CardTransformed)->model==card_token({Strike,1})&&last(RunHistoryEvent::CardTransformed)->detail==card_token({Bash,1})&&!last(RunHistoryEvent::CardTransformed)->flags);
}
static void capacity(){
 scenario="full 33 live owners, all 96 floors and 4096 used entries fit host capacity";clear_ui_progress(fixture);fixture.count=32;fixture.next_serial=34;fixture.current.serial=33;
 for(unsigned i=0;i<33;++i){auto&r=i<32?fixture.history[i]:fixture.current;r.serial=i+1;r.floor_count=UiHistoryFloors;r.deck_count=MaxDeck;r.relic_count=WorldRelicCapacity;r.facts.version=1;
  for(unsigned f=0;f<UiHistoryFloors;++f){r.floors[f].floor=uint16_t(f+1);r.floors[f].act=uint8_t(f%3);}
  for(unsigned id=0;id<WorldRelicCapacity;++id)r.relics[id]=uint16_t(id%WorldRelicKinds);
  auto&o=fixture.journal.owners[i];o.serial=i+1;for(auto&bits:o.known)bits=~uint32_t(0);
 }
 for(unsigned i=0;i<UiJournalEntries;++i)CHECK(fixture.journal.append(i%33,i%UiHistoryFloors,RunHistoryEvent::CardSkipped,card_token({Strike,uint8_t(i&1)})));
 const size_t n=encode_ui_progress(fixture,bytes,sizeof bytes);CHECK(n&&n<UiProgressCapacity&&decode_ui_progress(bytes,n,restored));CHECK(restored.journal.count==4096&&restored.count==32);
 std::printf("Worst NSH7 profile %zu / %u bytes; UiProgress=%zu journal=%zu tooltip=%zu.\n",n,UiProgressCapacity,sizeof(UiProgress),sizeof(UiHistoryJournal),sizeof(UiHistoryTooltip));
 scenario="ring loss marks original floor partial; owner indices are never recycled onto stale entries";ring={};uint32_t live[1]={1};CHECK(ring.bind(1,live,1)==0);ring.owners[0].observe(0,true);
 for(unsigned i=0;i<UiJournalEntries;++i)CHECK(ring.append(0,0,RunHistoryEvent::CardSkipped,card_token({Strike,0})));
 ring.owners[0].observe(1,true);CHECK(ring.append(0,1,RunHistoryEvent::CardSkipped,card_token({Bash,0}))&&ring.owners[0].incomplete(0)&&ring.count==4096);
 live[0]=2;CHECK(ring.bind(2,live,1)==0&&ring.count==0&&ring.owner(1)<0);ring.owners[0].observe(0,true);
 CHECK(ring.amount(0,0,RunHistoryEvent::GoldSpent,70000)&&ring.amount(0,0,RunHistoryEvent::GoldSpent,5)&&ring.count==1&&ring.at(0).amount()==70005);
}
static void hover(){
 scenario="long real observed floor can be read entirely with keyboard and pointer without changing State";start();
 for(unsigned i=0;i<70;++i){UiCoreObserver observer(a);const int index=event_add_deck_card(a.game,{Strike,uint8_t(i&1)});CHECK(index>=0&&event_remove_deck_card(a.game,index));}
 a.sync_progress();a.finish_progress(true);a.open_history();for(unsigned i=0;i<50;++i)a.advance(33);
 a.pointer.visible=false;a.history_presentation.focus=0;a.render();CHECK(a.history_tooltip.source==1&&a.history_tooltip.maximum>5);
 CHECK(save(a.game,before));a.input(Key::Confirm);CHECK(a.history_tooltip.keyboard);a.input(Key::Down);CHECK(a.history_tooltip.offset==1);a.input(Key::Back);CHECK(!a.history_tooltip.keyboard&&a.history_open);
 a.render();const auto rect=a.history_tooltip;const int page=a.progress_page_scroll.position;
 a.pointer_move(rect.x+10,rect.y+10);a.render();a.pointer_scroll(4096);a.render();
 CHECK(a.history_tooltip.offset==a.history_tooltip.maximum&&a.progress_page_scroll.position==page);
 bool hit=false;for(int i=0;i<a.hit_count;++i)hit|=a.hits[i].kind==HitKind::HistoryJournalScroll;CHECK(hit);
 a.pointer_scroll(-4096);CHECK(a.history_tooltip.offset==0);a.pointer_move(319,239);a.render();
 CHECK(save(a.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
}
int main(int argc,char**argv){std::vector<uint8_t> resource;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);resource.resize(spire_test::pack_size(f));CHECK(std::fread(resource.data(),1,resource.size(),f)==resource.size());CHECK(!std::fclose(f));CHECK(a.init(resource.data(),resource.size(),{}));}
 choices();card_rewards();shop_and_retained_rooms();persistence();capacity();hover();
 std::printf("NSH7 actual event/reward/purchase history, retained rooms, bounded owners, migration, decode failure publication and scroll purity: %u checks.\n",checks);
}
