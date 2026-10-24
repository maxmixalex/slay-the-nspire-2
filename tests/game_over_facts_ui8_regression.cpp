#include "app.hpp"
#include "ui_core_observer.hpp"
#include "world_run.hpp"
#include "run_options.hpp"
#include "run_score.hpp"
#include "game_over_ui.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
static App a;
static UiProgress decoded,old;
static UiRunHistory score_record;
static uint8_t bytes[UiProgressCapacity],legacy[UiProgressCapacity];
static SaveData before,after;
static unsigned checks;
static const char*scenario="initial";
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"game_over_facts_ui8:%d [%s] %s phase=%u\n",__LINE__,scenario,#x,unsigned(a.game.phase));std::exit(1);}}while(0)
static void start(unsigned seed=0x7195c3){
 a.reset_presentation();a.title=a.paused=a.help=a.library=a.compendium_open=a.history_open=false;
 new_run(a.game,seed,false);CHECK(configure_run_options(a.game,RunMode::Custom,run_modifier_bit(RunModifier::Flight)));
 CHECK(world_set_seed(a.game,"NSPIRE-WORLD-017")&&world_begin_run(a.game));a.begin_progress();
 CHECK(a.progress.current.facts.version==1&&a.progress.current.facts.complete&&!a.progress.current.partial);
}
static void route(reference::PointType wanted){
 auto&s=a.game;const auto&m=s.world.maps[0];uint8_t parents[reference::MaxPoints],queue[reference::MaxPoints];
 for(auto&v:parents)v=reference::NoPoint;parents[m.root]=m.root;queue[0]=m.root;unsigned head=0,count=1,target=reference::NoPoint;
 while(head<count){const unsigned p=queue[head++];if(m.points[p].type==wanted){target=p;break;}
  for(unsigned i=0;i<m.points[p].children.used;++i){const unsigned next=m.points[p].children.slots[i];if(next!=reference::NoPoint&&parents[next]==reference::NoPoint){parents[next]=uint8_t(p);queue[count++]=uint8_t(next);}}}
 CHECK(target!=reference::NoPoint);uint8_t path[reference::MaxRows+2];unsigned length=0;
 for(unsigned p=target;p!=m.root;p=parents[p])path[length++]=uint8_t(p);
 // Routing supplies intermediate outcomes. These fixtures must never claim
 // a fully observed score. The target room and its actual commands are real.
 a.progress.current.partial=1;a.progress.current.facts.complete=0;
 for(unsigned i=length;i>0;--i){if(!world_can_travel(s)){s.original_events={};s.hp=s.max_hp;if(combat_phase(s.phase)||s.phase==Phase::OriginalEventChoice)s.phase=Phase::Reward;CHECK(world_finish_room(s));}
  {UiCoreObserver observer(a);CHECK(world_choose_point(s,path[i-1]));}}
 CHECK(a.progress.current.floors[a.progress.current.floor_count-1].point==target);
}
static void score_and_badges(){
 scenario="pinned ScoreUtility arithmetic and end-room exclusions";score_record={};auto&r=score_record;r.serial=1;r.facts.version=r.facts.complete=1;r.outcome=1;r.floor_count=6;r.floor=6;
 for(unsigned i=0;i<6;++i)r.floors[i].act=uint8_t(i/2);r.gold_gained=1234;r.facts.elite_rooms=4;r.facts.boss_rooms=3;r.ascension=5;
 auto score=ui_run_score(r);CHECK(score.complete&&score.floor_points==120&&score.gold_points==12&&score.elites==4&&score.bosses==3&&score.total==948);
 r.facts.last_room=unsigned(WorldRoom::Elite);score=ui_run_score(r);CHECK(score.elites==3&&score.total==873);
 r.facts.last_room=unsigned(WorldRoom::Boss);r.outcome=2;CHECK(ui_run_score(r).bosses==2&&ui_run_score(r).total==798);
 r={};r.serial=1;r.facts.version=r.facts.complete=1;r.gold_gained=4500;r.ascension=4;CHECK(ui_run_score(r).total==62); // binary64 truncation, not rational 63.
 scenario="actual solo badge conditions and partial-history bounds";r={};r.serial=1;r.outcome=1;r.facts.version=r.facts.complete=1;r.max_hp=130;r.gold=600;r.seconds=1800;r.deck_count=5;
 for(unsigned i=0;i<5;++i)r.deck[i].card={Anger,0};r.facts.combo=1;r.facts.shop_spent=1000;r.facts.perfect_bosses=3;r.facts.boss_rooms=3;r.facts.elite_rooms=9;
 CHECK(ui_run_badge_rarity(r,UiRunBadge::Combo)==1&&ui_run_badge_rarity(r,UiRunBadge::Glutton)==3);
 CHECK(ui_run_badge_rarity(r,UiRunBadge::Highlander)==0&&ui_run_badge_rarity(r,UiRunBadge::Honed)==1);
 CHECK(ui_run_badge_rarity(r,UiRunBadge::Elite)==3&&ui_run_badge_rarity(r,UiRunBadge::Perfect)==3);
 CHECK(ui_run_badge_rarity(r,UiRunBadge::KaChing)==1&&ui_run_badge_rarity(r,UiRunBadge::MoneyMoney)==3);
 CHECK(ui_run_badge_rarity(r,UiRunBadge::Speedy)==3&&ui_run_badge_rarity(r,UiRunBadge::TinyDeck)==3);
 r.floor_count=2;for(unsigned i=0;i<2;++i){r.floors[i].room=unsigned(WorldRoom::Rest);r.floors[i].rest_choices=1;}
 CHECK(ui_run_badge_rarity(r,UiRunBadge::Restful)==1&&!ui_run_badge_rarity(r,UiRunBadge::Restless));
 r.floors[1].rest_choices=2;CHECK(!ui_run_badge_rarity(r,UiRunBadge::Restful));r.floors[0].rest_choices=2;CHECK(ui_run_badge_rarity(r,UiRunBadge::Restless)==1);
 r.partial=1;CHECK(!ui_run_score(r).complete&&!ui_daily_score(r)&&!ui_run_badge_rarity(r,UiRunBadge::Perfect)&&!ui_run_badge_rarity(r,UiRunBadge::Restless));
 CHECK(ui_run_badge_rarity(r,UiRunBadge::KaChing)==1&&ui_run_badge_rarity(r,UiRunBadge::Honed)==1); // positive facts and final deck remain real.
}
static void observations(){
 scenario="merchant actual purchase and remove spending";start();route(reference::PointType::Shop);a.game.gold=9999;
 const unsigned price=world_shop_price(a.game,0);{UiCoreObserver observer(a);CHECK(buy(a.game,0));}
 CHECK(a.progress.current.facts.shop_spent==price);{UiCoreObserver observer(a);CHECK(!buy(a.game,0));}CHECK(a.progress.current.facts.shop_spent==price);
 const unsigned removal=remove_price(a.game);{UiCoreObserver observer(a);CHECK(buy(a.game,ShopRemove));}
 CHECK(a.progress.current.facts.shop_spent==price);{UiCoreObserver observer(a);CHECK(remove_card(a.game,0));}CHECK(a.progress.current.facts.shop_spent==price+removal);
 scenario="rest cancellation and actual accepted choice";start(0x7195c4);route(reference::PointType::Rest);unsigned smith=99,heal=99;
 for(unsigned i=0;i<world_rest_option_count(a.game);++i){const auto*k=world_rest_option_key(a.game,i);if(!std::strcmp(k,"SMITH"))smith=i;if(!std::strcmp(k,"HEAL"))heal=i;}
 CHECK(smith!=99&&heal!=99);auto&f=a.progress.current.floors[a.progress.current.floor_count-1];
 {UiCoreObserver observer(a);CHECK(world_rest_choose(a.game,smith));CHECK(world_rest_cancel(a.game));}CHECK(f.rest_choices==0);
 a.game.hp=a.game.max_hp/2;{UiCoreObserver observer(a);CHECK(world_rest_choose(a.game,heal));}CHECK(f.rest_choices==1);
 scenario="unblocked HP, first-twenty finished-card command, discoveries";clear_ui_progress(a.progress);start(0x7195c5);uint8_t candidates[reference::Columns]{};CHECK(world_available_points(a.game,candidates,reference::Columns));
 {UiCoreObserver observer(a);CHECK(world_choose_point(a.game,candidates[0]));}CHECK(a.game.phase==Phase::Combat);
 auto&combat=a.progress.current.floors[a.progress.current.floor_count-1];CHECK(combat.facts==1);
 a.game.block=10;{UiCoreObserver observer(a);CHECK(player_hp_loss(a.game,1,true,true)==0);}CHECK(combat.facts==1);
 {UiCoreObserver observer(a);CHECK(player_hp_loss(a.game,1,false,false)==1);}CHECK(combat.facts==3);
 a.game.energy=99;for(unsigned i=0;i<20;++i){UiCoreObserver observer(a);queue_extension(a.game,XOp::EnemyGenerateCard,Defend,1025);resolve_pending(a.game);CHECK(play(a.game,int(a.game.hand.count)-1,0));resolve_pending(a.game);
  CHECK(a.progress.current.facts.combo==(i==19));}
 const unsigned earned=a.progress.current.gold_gained;{UiCoreObserver observer(a);gain_gold(a.game,123);}CHECK(a.progress.current.gold_gained==earned+123&&combat.gold_gained>=123);
 const unsigned cards=a.progress.current_discoveries.count[0];unsigned unseen=0;while(unseen<CardKinds&&(a.progress.discovered_cards[unseen/8]&(1u<<(unseen%8))))++unseen;CHECK(unseen<CardKinds);a.discover_card(CardKind(unseen));a.discover_card(CardKind(unseen));CHECK(a.progress.current_discoveries.count[0]==cards+1);
 CHECK(a.progress.current_discoveries.count[3]>0); // actual encounter spawn.
 // A10's two bosses share a point-level damage counter. A hit after an
 // already completed flawless boss must also disqualify that earlier boss.
 scenario="same-point second boss invalidates earlier Perfect credit";
 combat.facts=uint8_t(1|(1u<<2));a.progress.current.facts.boss_rooms=1;a.progress.current.facts.perfect_bosses=1;
 {UiCoreObserver observer(a);CHECK(player_hp_loss(a.game,1,false,false)==1);}
 CHECK(combat.facts==7&&a.progress.current.facts.perfect_bosses==0);
 combat.facts=3;
}
static void persistence_and_navigation(){
 scenario="NSH6 full tail and minimal old-prefix compatibility";a.finish_progress(true);a.progress.test_subject_kills=4;a.progress.architect_damage=948;
 const size_t n=encode_ui_progress(a.progress,bytes,sizeof bytes);CHECK(n&&bytes[3]=='6'&&decode_ui_progress(bytes,n,decoded));
 CHECK(decoded.current.facts.combo==1&&decoded.current.floors[decoded.current.floor_count-1].facts==3);
 CHECK(decoded.current_discoveries.count[0]==a.progress.current_discoveries.count[0]&&decoded.architect_damage==948&&decoded.test_subject_kills==4);
 CHECK(!std::memcmp(&decoded.current_discoveries,&a.progress.current_discoveries,sizeof(UiRunDiscoveries)));
 UiProgressSummary summary;CHECK(decode_ui_progress_summary(bytes,n,summary)&&summary.exists);CHECK(!decode_ui_progress(bytes,n-1,decoded));
 old=a.progress;old.current.facts={};for(auto&r:old.history)r.facts={};old.current_discoveries={};old.architect_damage=0;
 const size_t old_size=encode_ui_progress(old,legacy,sizeof legacy);CHECK(old_size&&legacy[3]=='5'&&decode_ui_progress(legacy,old_size,decoded));
 CHECK(decoded.current.facts.version==0&&decoded.current_discoveries.count[0]==0&&decoded.architect_damage==0&&decoded.test_subject_kills==4);
 scenario="legacy resumed prefix stays incomplete";start(0x7195c6);a.progress.current.facts={};a.progress.current_discoveries={};a.begin_progress(true);CHECK(a.progress.current.facts.version==1&&!a.progress.current.facts.complete&&!ui_run_score(a.progress.current).complete);
 scenario="source release hotkeys and run-history return preserve simulation";start(0x7195c7);
 {UiCoreObserver observer(a);player_kill(a.game,true);resolve_pending(a.game);}CHECK(a.game.phase==Phase::Dead&&a.progress.current.outcome==2&&validate(a.game)&&save(a.game,before));
 a.keyboard_only=true;const uint16_t select=a.input_bindings.keys[1][unsigned(InputCommand::Select)],confirm=a.input_bindings.keys[1][unsigned(InputCommand::Confirm)];
 CHECK(a.physical_key(select,true)&&!a.game_over_summary);CHECK(a.physical_key(select,true)&&!a.game_over_summary);CHECK(a.physical_key(select,false)&&a.game_over_summary);
 CHECK(a.physical_key(confirm,true)&&!a.history_open);CHECK(a.physical_key(confirm,false)&&a.history_open&&a.history_from_game_over&&!a.compendium_open);
 CHECK(a.history_at(a.history_index)->serial==a.progress.current.serial);a.input(Key::Back);CHECK(!a.history_open&&!a.compendium_open&&a.game_over_summary);
 // A modal appearing while a button is held must consume that release.
 CHECK(a.physical_key(select,true));a.paused=true;CHECK(a.physical_key(select,false)&&!a.title);a.paused=false;
 CHECK(save(a.game,after)&&after.size==before.size&&!std::memcmp(before.bytes,after.bytes,before.size));
 CHECK(a.physical_key(select,true)&&!a.title);CHECK(a.physical_key(select,false)&&a.title);
 CHECK(save(a.game,after)&&after.size==before.size&&!std::memcmp(before.bytes,after.bytes,before.size));
}
int main(){clear_ui_progress(a.progress);a.progress_loaded=true;score_and_badges();observations();persistence_and_navigation();
 std::printf("Game-over observed facts: %u checks, UiFloorHistory=%zu UiRunFacts=%zu UiRunDiscoveries=%zu UiRunHistory=%zu UiProgress=%zu App=%zu\n",checks,sizeof(UiFloorHistory),sizeof(UiRunFacts),sizeof(UiRunDiscoveries),sizeof(UiRunHistory),sizeof(UiProgress),sizeof(App));}
