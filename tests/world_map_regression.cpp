#include "core.hpp"
#include "world_run.hpp"
#include "world_card_factory.hpp"
#include "original_events.hpp"
#include "world_map_layout.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace spire;
static State game,loaded,other;
static SaveData bytes,again;
static unsigned checks=0;
static void require(bool value,const char* message){++checks;if(!value){std::fprintf(stderr,"world map: %s\n",message);std::exit(1);}}
static void start(unsigned asc=0){
 new_run(game,0x7195c3,false,Character::Ironclad);
 require(world_set_seed(game,"NSPIRE-WORLD-017"),"canonical seed accepted");
 require(world_begin_run(game,reference::ActKind::Overgrowth,asc),"native run begins");
 require(game.phase==Phase::Event&&game.original_events.id==OriginalEventId::Neow,"first Neow entered automatically");
 require(game.world.total_floor==1&&game.world.room_open,"ancient is a visited floor");
}
static void persisted(){
 require(validate(game),"native state validates");require(save(game,bytes),"native state saves");
 require(load(loaded,bytes.bytes,bytes.size),"native state loads");require(save(loaded,again),"native state resaves");
 require(bytes.size==again.size&&!std::memcmp(bytes.bytes,again.bytes,bytes.size),"native save is byte exact");game=loaded;
}
// This harness supplies room outcomes to isolate map routing from battle and
// individual event scripts. It deliberately does not claim enemy AI parity.
static void finish_routing_fixture(){
 if(game.original_events.active){game.original_events=OriginalEventState{};}
 if(world_room_is_combat(game))game.phase=Phase::Reward;
 require(world_finish_room(game),"completed room returns through native run controller");
 require(world_valid_run(game),"routing invariants hold after completion");
}
static unsigned choose_first(){
 uint8_t candidates[reference::MaxPoints];unsigned count=world_available_points(game,candidates,reference::MaxPoints);
 require(count>0&&count<=reference::Columns,"reachable frontier has one to seven points");
 unsigned point=candidates[0];require(world_reachable(game,point),"frontier point reachable");
 require(world_choose_point(game,point),"frontier point enters");return point;
}
static void detached_offer_preview(){
 other=game;EventCardFacts f{};int value=6;
 f.enchantment=EventEnchantment::Sharp;f.enchantment_amount=4;
 require(event_card_offer_description_value(game,{Strike,0},f,"Damage",value)&&value==10,"detached Sharp uses supplied canonical damage");
 f.enchantment=EventEnchantment::Corrupted;value=5;
 require(event_card_offer_description_value(game,{Strike,0},f,"Damage",value)&&value==7,"detached multiplicative damage uses original floor rounding");
 f.enchantment=EventEnchantment::TezcatarasEmber;f.energy_offset=-1;value=99;
 require(event_card_offer_description_value(game,{Strike,0},f,"EnergyCost",value)&&value==0,"detached Ember displays its permanent energy offset");
 f.energy_offset=0;require(event_card_offer_description_value(game,{Whirlwind,0},f,"EnergyCost",value)&&value==-1,"detached energy preview preserves X");
 f={};f.science_type=uint8_t(CardType::Skill);f.rider=uint8_t(TinkerRider::Energized);f.enchantment=EventEnchantment::Nimble;f.enchantment_amount=3;
 value=8;require(event_card_offer_description_value(game,{EventCardMadScience,0},f,"Block",value)&&value==11,"detached Mad Science applies Nimble to its canonical skill block");
 require(event_card_offer_description_value(game,{EventCardMadScience,0},f,"CardType",value)&&value==int(CardType::Skill),"detached Mad Science preserves the generated type");
 require(event_card_offer_description_value(game,{EventCardMadScience,0},f,"TinkerTimeRider",value)&&value==int(TinkerRider::Energized),"detached Mad Science preserves the selected rider");
 require(event_card_offer_description_value(game,{EventCardMadScience,0},f,"EnergizedEnergy",value)&&value==2,"detached rider reuses event canonical variables");
 value=37;require(!event_card_offer_description_value(game,{Strike,0},f,"UnknownVariable",value)&&value==37,"unknown detached variable preserves caller canonical value");
 require(!std::memcmp(&game,&other,sizeof game),"detached previews never mutate the state or RNG");
 game.deck[0]={DefectGeneticAlgorithm,0};game.defect.genetic_deck[0]=17;game.event_cards.deck[0]={};game.event_cards.deck[0].enchantment=EventEnchantment::Nimble;game.event_cards.deck[0].enchantment_amount=3;value=1;
 require(character_deck_description_value(game,game.deck[0],0,"Block",value)&&value==21,"permanent growth is resolved before detached enchantment preview");game=other;
}
struct ObserverProbe {unsigned count=0;const State* expected=nullptr;Log last{};};
static void observe_log(void* opaque,const State&s,Log entry){
 auto& probe=*static_cast<ObserverProbe*>(opaque);++probe.count;probe.last=entry;
 require(probe.expected==&s,"observer receives the originating state without copying it");
 require(s.log_count&&s.logs[s.log_count-1].kind==entry.kind&&s.logs[s.log_count-1].a==entry.a&&s.logs[s.log_count-1].b==entry.b,"observer runs after the exact log has been appended");
}
static void scoped_core_observer(){
 Log previous_logs[24];for(unsigned i=0;i<24;i++)previous_logs[i]=game.logs[i];const auto previous_count=game.log_count;other=game;
 ObserverProbe outer{0,&game,{}},inner{0,&game,{}};
 CoreObserver original=set_core_observer({&outer,observe_log});
 for(int i=0;i<40;i++)record(game,LogKind::Gold,i);
 CoreObserver enclosing=set_core_observer({&inner,observe_log});record(game,LogKind::Gold,40);
 require(enclosing.context==&outer&&enclosing.on_log==observe_log,"observer setter returns the enclosing binding");
 set_core_observer(enclosing);record(game,LogKind::Gold,41);
 CoreObserver restored=set_core_observer({});record(game,LogKind::Gold,42);
 require(restored.context==&outer&&outer.count==41&&inner.count==1&&outer.last.a==41&&inner.last.a==40,"nested scopes restore their observer and receive logs beyond the 24-entry window");
 for(int i=0;i<43;i++)record(other,LogKind::Gold,i);
 require(game.log_count==other.log_count,"observer preserves the complete retained log count");
 for(unsigned i=0;i<game.log_count;i++)require(game.logs[i].kind==other.logs[i].kind&&game.logs[i].a==other.logs[i].a&&game.logs[i].b==other.logs[i].b,"observer preserves every retained log field");
 // Log has padding between kind and a. Independently constructed log entries
 // need not have equal padding, so compare the complete canonical encoding.
 require(save(game,bytes)&&save(other,again),"both observed and unobserved states remain serializable");
 require(bytes.size==again.size&&!std::memcmp(bytes.bytes,again.bytes,bytes.size),"record observer never changes any serialized game field or RNG state");
 set_core_observer(original);for(unsigned i=0;i<24;i++)game.logs[i]=previous_logs[i];game.log_count=previous_count;
}
static void combat_extra_reward_group(){
 start();finish_routing_fixture();
 while(game.deck_count)require(event_remove_deck_card(game,int(game.deck_count)-1),"replace fixture deck through permanent-card removal");
 require(event_add_deck_card(game,{TheHunt,0})>=0&&event_add_deck_card(game,{Strike,0})>=0,"add real Hunt and removable card instances before combat");
 choose_first();require(game.phase==Phase::Combat,"extra-reward fixture enters a real native combat");
 require(encounter_setup(game,EncounterId::NibbitsNormal,0),"use a two-enemy encounter with ordinary Fatal behavior");
 queue_extension(game,XOp::NPower,int(NPower::ForbiddenGrimoire),1);
 queue_extension(game,XOp::RPower,int(RPower::Royalties),9);resolve_pending(game);
 int hunt=-1;for(unsigned i=0;i<game.hand.count;++i)if(game.cards[game.hand.ids[i]].kind==TheHunt)hunt=int(i);
 require(hunt>=0,"first draw contains the actual Hunt instance");game.enemies[0].hp=1;
 require(play(game,hunt,0),"Hunt kills the first ordinary enemy through its attack command");
 unsigned extra_kind=99;int extra_value=0;
 require(encounter_combat_reward_count(game)==1&&encounter_combat_reward(game,0,extra_kind,extra_value)&&extra_kind==1&&extra_value==3,"Fatal appends one original three-card reward before combat ends");
 require(!game.silent.extra_rewards&&game.phase==Phase::Combat,"native Hunt has one saved reward owner while the other enemy remains");persisted();
 for(unsigned guard=0;guard<64&&game.phase==Phase::Combat;++guard){
  for(unsigned i=0;i<game.enemy_count;++i)if(enemy_allow_target(game,int(i)))damage_unpowered(game,int(i),9999);
  resolve_pending(game);
 }
 require(game.phase==Phase::WorldRelicChoice&&game.world_relics.choice==WorldRelicChoice::OfferReward,"Hunt, end-of-combat powers and main rewards open one group");
 const unsigned group=game.world_relics.choice_group;unsigned main_cards=0,hunt_cards=0,removals=0,royalties=0;
 for(unsigned i=0;i<game.world_relics.offer_count;++i){const auto&r=game.world_relics.rewards[game.world_relics.offers[i]];
  require(r.group==group,"every combat reward stays in the same selectable group");
  if(r.kind==1){require(r.count==3,"both main and Hunt rewards preserve three original options");if(r.creation_flags&WorldIsFromCombat)++main_cards;else ++hunt_cards;}
  if(r.kind==6)++removals;if(r.kind==4&&r.value==9)++royalties;
 }
 require(main_cards==1&&hunt_cards==1&&removals==1&&royalties==1,"main CardReward, Hunt, Grimoire and Royalties all survive the original reward handoff");
 require(!game.silent.extra_rewards&&!game.necro.grimoire_pending,"transferred native rewards cannot also reopen through historical counters");persisted();
 require(world_relic_select(game,nullptr,0)&&game.phase==Phase::Reward,"all optional extras can be skipped together");
 require(leave_rewards(game)&&game.phase==Phase::Map,"closing the complete group returns to the next native map frontier");persisted();
}
static void visited_room_history(){
 start();require(game.world.visited_rooms[0][game.world.current_point]==unsigned(WorldRoom::Event),"first ancient records its original room type");
 const auto&map=game.world.maps[0];uint8_t parents[reference::MaxPoints],queue[reference::MaxPoints];
 for(auto&v:parents)v=reference::NoPoint;parents[map.root]=map.root;queue[0]=map.root;unsigned head=0,count=1,target=reference::NoPoint;
 while(head<count){unsigned at=queue[head++];if(map.points[at].type==reference::PointType::Unknown){target=at;break;}
  const auto&children=map.points[at].children;for(unsigned i=0;i<children.used;++i){unsigned next=children.slots[i];if(next!=reference::NoPoint&&parents[next]==reference::NoPoint){parents[next]=uint8_t(at);queue[count++]=uint8_t(next);}}
 }
 require(target!=reference::NoPoint,"fixture has a reachable original unknown node");uint8_t path[reference::MaxRows+2];unsigned length=0;
 for(unsigned at=target;at!=map.root;at=parents[at]){require(length<sizeof path,"unknown path is bounded");path[length++]=uint8_t(at);}
 finish_routing_fixture();while(length){unsigned at=path[--length];if(at==target)game.world.unknown_monster_bits=0x3f800000U;
  require(world_choose_point(game,at),"follow original links to an unknown room");if(at!=target)finish_routing_fixture();
 }
 require(game.phase==Phase::Combat&&game.world.visited_rooms[0][target]==unsigned(WorldRoom::Monster),"unknown room remembers its resolved combat type");
 require(game.world.maps[0].points[target].type==reference::PointType::Unknown&&world_map_visible_type(game,target)==reference::PointType::Monster,"revealing an unknown icon does not rewrite the original map graph");persisted();
 other=game;game.world.visited_rooms[0][target]=unsigned(WorldRoom::Boss);require(!world_valid_run(game),"impossible unknown-to-boss history is rejected");game=other;
 game.world.visited_rooms[0][target]=0;require(!world_valid_run(game),"visited points require their original room history");game=other;
 // A parent event owns the map visit even when it starts a nested encounter.
 start();const unsigned event_point=game.world.current_point;game.original_events=OriginalEventState{};
 require(original_event_begin(game,OriginalEventId::FakeMerchant,game.seed,0),"prepare an event-owned combat at the visited event coordinate");
 require(world_enter_event_combat(game,"FakeMerchantEventEncounter",true)&&game.enemy_world.encounter==EncounterId::FakeMerchantEventEncounter,"event enters its native combat room");
 require(game.world.room_type==unsigned(WorldRoom::Monster)&&game.world.parent_room_type==unsigned(WorldRoom::Event)&&game.world.visited_rooms[0][event_point]==unsigned(WorldRoom::Event),"nested combat cannot replace the first map-room type");persisted();
}
static void room_proceed_boundaries(){
 // Successful ordinary rest options stay in the room after the original
 // synchronizer clears its options. Only the separate Proceed action travels.
 start();game.original_events=OriginalEventState{};game.world.room_type=uint8_t(WorldRoom::Rest);game.phase=Phase::Camp;
 game.world.rest_count=2;game.world.rest_options[0]=uint8_t(WorldRestOption::Heal);game.world.rest_options[1]=uint8_t(WorldRestOption::Smith);
 require(world_rest_option_visible(game,0)&&world_rest_option_visible(game,1)&&!world_rest_option_visible(game,2),"new rest exposes exactly its original option slots");
 require(world_rest_choose(game,1)&&game.phase==Phase::Upgrade,"ordinary smith opens the actual card upgrade selection");
 require(world_rest_cancel(game)&&!world_rest_can_proceed(game)&&world_rest_option_visible(game,1),"cancelling ordinary smith does not finish or remove its option");
 require(world_rest_choose(game,1)&&upgrade_card(game,0),"ordinary smith completes through the shared upgrade path");
 require(game.deck[0].upgraded&&game.phase==Phase::Camp&&game.world.room_open&&!game.world.rest_pending&&world_rest_can_proceed(game),"ordinary smith finishes its option and reveals Proceed without closing the room");
 require(!world_rest_option_visible(game,0)&&!world_rest_option_visible(game,1)&&!world_rest_option_enabled(game,0)&&!world_rest_choose(game,0),"ordinary completion removes every remaining option");persisted();
 require(game.phase==Phase::Camp&&world_rest_can_proceed(game)&&!world_rest_option_visible(game,1),"ordinary completed rest resumes with Proceed and its removed options intact");
 const unsigned rooms_before=game.world.rooms_completed;
 require(world_rest_proceed(game)&&game.phase==Phase::Map&&!game.world.room_open&&game.world.rooms_completed==rooms_before+1,"ordinary rest travels exactly once through Proceed");
 require(!world_rest_proceed(game),"completed rest cannot proceed twice");persisted();

 // AfterRest can pause in a real reward group. That group must complete before
 // the room removes its options, and Proceed must not repeat its heal or RNG.
 start();game.original_events=OriginalEventState{};game.world.room_type=uint8_t(WorldRoom::Rest);game.phase=Phase::Camp;
 game.world.rest_count=2;game.world.rest_options[0]=uint8_t(WorldRestOption::Heal);game.world.rest_options[1]=uint8_t(WorldRestOption::Smith);
 require(gain_world_relic(game,WorldRelicDreamCatcher),"obtain actual delayed rest reward listener");game.hp=30;
 require(world_rest_choose(game,0)&&game.phase==Phase::WorldRelicChoice&&game.world.rest_pending==1&&game.world.finish_pending,"ordinary heal awaits its Dream Catcher card reward");
 require(!world_rest_can_proceed(game)&&!world_rest_proceed(game),"pending rest reward blocks room travel");persisted();
 require(world_relic_select(game,nullptr,0)&&game.phase==Phase::Camp&&!game.world.rest_pending&&!world_relic_pending(game),"finishing a rest reward resumes its original Camp continuation");
 require(world_rest_can_proceed(game)&&!world_rest_option_visible(game,0)&&!world_rest_option_visible(game,1),"awaited ordinary rest clears its options before exposing Proceed");persisted();
 const int healed_hp=game.hp;const unsigned reward_calls=world_rng(game,WorldRng::Rewards).calls;
 require(world_rest_proceed(game)&&game.phase==Phase::Map&&game.hp==healed_hp&&world_rng(game,WorldRng::Rewards).calls==reward_calls,"Proceed does not repeat the completed heal or card reward draw");persisted();

 // Reach an actual map-generated treasure room. Its relic has already been
 // prepared on room entry, but an unopened chest may be left without any of
 // the chest's gold, relic-acquisition callbacks or opening RNG being run.
 start();finish_routing_fixture();
 for(unsigned guard=0;guard<20&&game.phase!=Phase::Treasure;++guard){choose_first();if(game.phase!=Phase::Treasure)finish_routing_fixture();}
 require(game.phase==Phase::Treasure&&game.world.room_open&&!game.treasure_state&&game.world.treasure_relic!=NoWorldRelic,"original map enters a closed chest with its prepared relic");persisted();
 const int gold=game.gold;const unsigned relics=game.world_relics.count;
 reference::Random streams[WorldRngCount];for(unsigned i=0;i<WorldRngCount;++i)streams[i]=game.world.rng[i];
 require(leave_treasure(game)&&game.phase==Phase::Map&&!game.world.room_open,"native unopened treasure accepts Proceed");
 require(game.gold==gold&&game.world_relics.count==relics&&!game.world.treasure_gold_claimed,"leaving a closed chest gives no opening gold or relic");
 for(unsigned i=0;i<WorldRngCount;++i){const auto&before=streams[i];const auto&after=game.world.rng[i];
  require(before.calls==after.calls&&before.good==after.good,"skipping a closed chest consumes no named RNG draw");
  for(unsigned j=0;j<4;++j)require(before.words[j]==after.words[j],"closed-chest travel preserves every named generator word");
 }
 require(!leave_treasure(game)&&!open_treasure(game)&&!take_treasure(game),"a skipped chest cannot be opened or claimed after travel");persisted();
}
int main(){
 room_proceed_boundaries();visited_room_history();start();require(game.potion_slots==3,"default run has three potion slots");detached_offer_preview();scoped_core_observer();persisted();
 for(unsigned a=0;a<3;++a){const auto& rooms=game.world.rooms[a];
  require(rooms.ready&&rooms.elite_count==15&&rooms.event_count>0,"all three room queues generated up front");
  require(rooms.boss_count==1,"base ascension has one boss per act");
 }
 other=game;auto saved_rng=game.world.rng[unsigned(WorldRng::UpFront)];
 require(!world_choose_point(game,reference::MaxPoints),"invalid point rejected");
 require(!std::memcmp(&game,&other,sizeof game),"rejected point consumes no state or random draw");
 require(saved_rng.calls==game.world.rng[unsigned(WorldRng::UpFront)].calls,"preview consumes no upfront RNG");
 game.world.unknown_monster_bits=0x3e99999aU;game.world.unknown_shop_bits=0x3d75c28fU;
 game.world.potion_drop_bits=0xbdcccccdU;persisted();
 require(game.world.potion_drop_bits==0xbdcccccdU,"negative potion odds survive saves without clamping");
 finish_routing_fixture();
 require(game.phase==Phase::Map&&game.world.rooms[0].event_cursor==1,"ancient consumes event-room visit cursor");
 unsigned visited=1,previous_act=0,act_changes=0,bosses=0;
 while(!game.world.final_event&&visited<64){
  unsigned point=choose_first();++visited;const auto& map=game.world.maps[game.world.act_index];
  if(map.points[point].type==reference::PointType::Boss)++bosses;
  finish_routing_fixture();
  if(game.world.act_index!=previous_act){
   ++act_changes;previous_act=game.world.act_index;
   require(game.world.current_point==reference::NoPoint&&game.phase==Phase::Map,"later ancient waits on the map");
   require(game.world.unknown_monster_bits==0x3dcccccdU&&game.world.unknown_elite_bits==0xbf800000U,"unknown odds reset per act");
  }
 }
 require(act_changes==2&&bosses==3&&game.world.final_event,"three original acts reach Architect");
 require(game.original_events.id==OriginalEventId::TheArchitect,"final room is Architect dialogue");
 unsigned final_floor=game.world.total_floor;finish_routing_fixture();
 require(game.phase==Phase::Complete&&game.world.finished&&game.world.total_floor==final_floor,"ending does not invent an extra map floor");

 start(10);require(game.potion_slots==2,"ascension reduces potion capacity");
 require(game.world.rooms[2].boss_count==2&&game.world.rooms[2].bosses[0]!=game.world.rooms[2].bosses[1],"A10 queues two distinct final bosses");
 require(game.deck[game.deck_count-1].kind==EventCardAscendersBane,"ascension curse joins starting deck");persisted();
 other=game;game.world.visited[0][game.world.current_point]=0;require(!world_valid_run(game),"missing current visited point rejected");game=other;
 game.world.shop_auto_cursor=15;require(!world_valid_run(game),"bad Parasol cursor rejected");game=other;
 game.world.rest_count=MaxWorldRestOptions+1;require(!world_valid_run(game),"bad rest option count rejected");game=other;

 // A direct native Camp fixture verifies cancellation, the two-card atomic
 // cook selection, and Miniature Tent's per-option use without rerolling a map.
 game.original_events=OriginalEventState{};game.world.room_type=uint8_t(WorldRoom::Rest);game.phase=Phase::Camp;
 game.world.rest_count=2;game.world.rest_options[0]=uint8_t(WorldRestOption::Heal);game.world.rest_options[1]=uint8_t(WorldRestOption::Smith);
 require(!world_rest_can_proceed(game)&&!world_rest_proceed(game),"rest cannot be skipped before any option is completed");
 require(world_rest_choose(game,1)&&game.phase==Phase::Upgrade,"native smith opens deck selection");
 require(world_rest_cancel(game)&&game.phase==Phase::Camp&&!game.world.rest_used,"cancelled smith leaves option unused");
 require(gain_world_relic(game,WorldRelicMiniatureTent),"obtain tent fixture");
 game.hp=30;require(world_rest_choose(game,0),"rest heals through native option");
 require(game.phase==Phase::Camp&&(game.world.rest_used&1)&&!world_rest_option_visible(game,0)&&world_rest_option_visible(game,1)&&!world_rest_option_enabled(game,0)&&world_rest_option_enabled(game,1),"tent removes its used option and preserves unused camp options");
 require(world_rest_can_proceed(game),"tent exposes Proceed even while another option remains usable");
 require(gain_world_relic(game,WorldRelicMeatCleaver),"obtain cook fixture");
 game.world.rest_options[2]=uint8_t(WorldRestOption::Cook);game.world.rest_count=3;
 unsigned deck=game.deck_count,maxhp=game.max_hp;
 require(world_rest_choose(game,2)&&game.phase==Phase::WorldRelicChoice,"cook pauses for exact selection");
 uint16_t eligible[MaxDeck];unsigned available=world_relic_eligible(game,eligible,MaxDeck);require(available>=2,"cook has removable cards");
 require(!world_rest_selection_allowed(game,1),"cook refuses a one-card partial operation");
 require(world_relic_select(game,eligible,2),"cook accepts exactly two instances");
 require(game.deck_count==deck-2&&game.max_hp==maxhp+5&&game.phase==Phase::Camp,"cook removes two then gains five maximum HP");
 require(world_rest_proceed(game)&&game.phase==Phase::Map&&!game.world.room_open,"tent can leave through the ordinary room controller after optional actions");persisted();

 // Exercise the shared engine through a real first combat, including its
 // complete first draw, enemy side and native reward handoff. The final damage
 // is an explicit fixture command, not a player strategy or enemy-parity claim.
 start();finish_routing_fixture();
 require(gain_world_relic(game,WorldRelicAnchor)&&gain_world_relic(game,WorldRelicVajra)&&gain_world_relic(game,WorldRelicPaelsWing)&&gain_world_relic(game,WorldRelicDriftwood),"obtain shared-engine fixtures before room entry");
 require(gain_world_relic(game,WorldRelicChosenCheese)&&gain_world_relic(game,WorldRelicMeatOnTheBone)&&gain_world_relic(game,WorldRelicRedSkull),"obtain ordered victory listeners");
 choose_first();require(game.phase==Phase::Combat&&game.turn==1&&game.enemy_world.active,"map enters a native encounter");
 require(game.block==10&&game.strength==1,"first-turn clear retains Anchor and native Vajra runs exactly once");
 require(game.hand.count==5&&game.playing<0&&!game.effect_count,"first draw and native hooks completely resume");persisted();
 require(end_turn(game),"end player side through native continuations");
 require(game.phase==Phase::Combat&&game.turn==2&&!game.expansion.enemy_side&&!game.enemy_world.side&&!game.effect_count,"enemy side completes before the next player turn");persisted();
 other=game;game.expansion.skip_enemy=1;require(end_turn(game),"extra player turn resumes through native continuation");
 require(game.phase==Phase::Combat&&game.turn==3&&!game.expansion.enemy_side&&!game.effect_count,"extra player turn completes its normal draw");
 for(unsigned i=0;i<game.enemy_count;i++){
  const auto&a=game.enemy_world.models[i];const auto&b=other.enemy_world.models[i];
  require(a.node==b.node&&a.previous==b.previous&&a.history_count==b.history_count&&!std::memcmp(a.history,b.history,sizeof a.history)&&!std::memcmp(&a.rng,&b.rng,sizeof a.rng),"extra player turn preserves enemy intent, history and RNG");
 }
 persisted();
 // Gold from Royalties is a room reward. It must not trigger gold pickup
 // listeners while the power's AfterCombatEnd callback is still executing.
 queue_extension(game,XOp::RPower,int(RPower::Royalties),7);resolve_pending(game);
 const unsigned ending_max_hp=game.max_hp;const int ending_hp=game.max_hp/2;
 const int old_hp=game.hp;game.hp=int16_t(ending_hp);world_relic_hook(game,WorldRelicHook::AfterHpChanged,old_hp,game.hp,-1);resolve_pending(game);
 require(game.strength==4,"Red Skull activates before the ending fixture");
 const int gold_before_victory=game.gold;
 for(unsigned guard=0;guard<64&&game.phase==Phase::Combat;guard++){
  for(unsigned i=0;i<game.enemy_count;i++)if(enemy_allow_target(game,int(i)))damage_unpowered(game,int(i),9999);
  resolve_pending(game);
 }
 require(game.phase==Phase::WorldRelicChoice&&game.world_relics.choice==WorldRelicChoice::OfferReward,"Royalties remains a separately claimable reward");
 require(game.gold==gold_before_victory&&game.world.combat_bonus_gold==7,"Royalties does not grant gold during combat-end callbacks");
 require(game.max_hp==ending_max_hp+1&&game.hp==ending_hp+7,"Cheese increases HP before Meat on the Bone tests its threshold, then Burning Blood heals");
 require(game.world.combat_ending&&game.strength==0&&game.block==0,"combat-end power removal precedes victory healing without reactivating Red Skull");persisted();
 const unsigned combat_group=game.world_relics.choice_group;
 require(combat_group&&game.world.reward_count==0&&game.reward_claimed==7&&game.world.reward_relic_claimed,"main rewards transfer once to the canonical combat group");
 int royalties_offer=-1,main_card=-1;
 for(unsigned i=0;i<game.world_relics.offer_count;i++){
  const auto index=game.world_relics.offers[i];const auto&r=game.world_relics.rewards[index];
  require(r.group==combat_group,"all main and extra combat rewards share one freely selectable group");
  if(r.kind==4&&r.value==7)royalties_offer=int(i);
  if(r.kind==1&&(r.creation_flags&WorldIsFromCombat))main_card=int(index);
 }
 require(royalties_offer>=0&&main_card>=0,"Royalties and original main card reward coexist");
 uint16_t gold_offer=uint16_t(royalties_offer);require(world_relic_select(game,&gold_offer,1),"Royalties gold can be selected before main rewards");
 require(game.gold==gold_before_victory+7,"claiming Royalties grants its exact amount");
 require(game.phase==Phase::WorldRelicChoice&&game.world_relics.choice==WorldRelicChoice::OfferReward&&game.world_relics.rewards[main_card].count>=3,"taking an extra returns to the same combat reward group");
 int card_offer=-1;for(unsigned i=0;i<game.world_relics.offer_count;i++)if(game.world_relics.offers[i]==main_card)card_offer=int(i);
 require(card_offer>=0,"unclaimed main card reward stays selectable after an extra");
 uint16_t card_choice=uint16_t(card_offer);require(world_relic_select(game,&card_choice,1),"main card reward opens from shared reward list");
 require(world_card_reward_alternatives(game)==3,"Pael's Wing and Driftwood are offered for the actual native card reward");
 const unsigned reward_rng=world_rng(game,WorldRng::Rewards).calls;const unsigned claim_mask=game.reward_claimed;
 const int reward_gold=game.last_gold;const auto reward_potion=game.reward_potion;const unsigned reroll_deck=game.deck_count;
 require(world_take_card_reward_alternative(game,2),"Driftwood rerolls the current native card reward");
 require(game.phase==Phase::WorldRelicChoice&&game.world_relics.choice==WorldRelicChoice::OfferCard&&(game.world_relics.rewards[main_card].flags&4)&&!(game.world_relics.rewards[main_card].flags&16)&&world_card_reward_alternatives(game)==1,"reroll preserves the same offer and removes only its reroll alternative");
 require(world_rng(game,WorldRng::Rewards).calls>reward_rng&&game.reward_claimed==claim_mask&&game.last_gold==reward_gold&&game.reward_potion==reward_potion&&game.deck_count==reroll_deck,"reroll changes card RNG without claiming cards or regenerating gold and potions");persisted();
 other=game;require(!world_take_card_reward_alternative(game,2)&&!std::memcmp(&game,&other,sizeof game),"saved reward cannot be rerolled twice");
 unsigned old_deck=game.deck_count;require(world_take_card_reward_alternative(game,1),"sacrifice consumes the card reward");
 require((game.world_relics.rewards[main_card].flags&1)&&game.deck_count==old_deck&&world_relic_counter(game,WorldRelicPaelsWing)==1,"sacrifice records one reward without adding a card");
 other=game;require(!world_take_card_reward_alternative(game,1)&&!std::memcmp(&game,&other,sizeof game),"already consumed reward cannot be sacrificed twice");persisted();
 require(world_relic_select(game,nullptr,0)&&game.phase==Phase::Reward&&!game.world_relics.group_active[combat_group-1],"proceed closes the entire combat group");
 other=game;require(!claim_reward_gold(game)&&!claim_reward_potion(game)&&!world_claim_reward_relic(game)&&!world_claim_reward_card(game,0)&&!std::memcmp(&game,&other,sizeof game),"transferred main rewards cannot be claimed a second time through historical APIs");
 require(leave_rewards(game)&&game.phase==Phase::Map,"shared reward completion returns through native room controller");persisted();

 // Death prevention is an awaited part of damage, and automatic potion use
 // must keep the original DamageResult separate from the later healing.
 start();finish_routing_fixture();game.potion_slots=8;for(auto&p:game.potions)p=Potion::Empty;game.potions[7]=Potion::FairyInABottle;
 require(gain_world_relic(game,WorldRelicLizardTail)&&gain_world_relic(game,WorldRelicBeltBuckle)&&gain_world_relic(game,WorldRelicReptileTrinket),"obtain ordered death-prevention and potion listeners");
 require(gain_world_relic(game,WorldRelicCentennialPuzzle)&&gain_world_relic(game,WorldRelicSelfFormingClay),"obtain damage-received listeners");
 choose_first();game.hp=1;game.block=0;const auto losses_before=game.ironclad.hp_losses_combat;
 require(player_hp_loss(game,25,true,true,-1,0)==1,"lethal DamageResult reports capped original loss");
 require(game.hp==0&&game.phase==Phase::Combat&&game.effect_count&&game.potions[7]==Potion::FairyInABottle,"lethal callbacks remain saved before potion prevention");persisted();
 other=game;game.effects[0].kind=EffectKind(255);
 require(!validate(game),"allowing a pending Combat save still rejects an unknown continuation operation");game=other;
 resolve_pending(game);
 require(game.phase==Phase::Combat&&game.hp==game.max_hp*3/10&&game.potions[7]==Potion::Empty,"Fairy from an extended slot heals through its potion wrapper");
 require(!world_relic_counter(game,WorldRelicLizardTail),"ordinary Fairy prevention precedes late Lizard Tail");
 require(game.dexterity==2&&game.strength==3&&game.world_relics.reptile_trinket==3,"automatic Fairy fires Belt Buckle and Reptile Trinket once");
 require(game.ironclad.hp_losses_combat==losses_before&&!world_relic_counter(game,WorldRelicCentennialPuzzle)&&!game.world_relics.self_forming_clay,"lethal damage does not invent a received callback after revival");persisted();
 player_kill(game);resolve_pending(game);
 require(game.phase==Phase::Combat&&game.hp==game.max_hp/2&&world_relic_counter(game,WorldRelicLizardTail)==1,"explicit non-force Kill uses the remaining Tail");
 require(game.ironclad.hp_losses_combat==losses_before&&!world_relic_counter(game,WorldRelicCentennialPuzzle)&&!game.world_relics.self_forming_clay,"explicit Kill is not reported as damage");persisted();
 game.potions[7]=Potion::FairyInABottle;player_kill(game,true);resolve_pending(game);
 require(game.phase==Phase::Dead&&game.world.finished&&game.potions[7]==Potion::FairyInABottle,"force Kill bypasses death prevention");persisted();

 // The card choice suspends inside the bottle's effect. PotionDone operands
 // and potion_active must survive an actual save before AfterPotionUsed.
 start();finish_routing_fixture();require(gain_world_relic(game,WorldRelicReptileTrinket),"obtain potion-resume listener");choose_first();
 for(auto&p:game.potions)p=Potion::Empty;game.potions[0]=Potion::ColorlessPotion;
 require(use_potion(game,0,0)&&game.phase==Phase::ExtendedChoice&&game.expansion.potion_active,"manual card potion suspends at its actual offer");
 require(game.strength==0,"AfterPotionUsed waits for the offered card");
 bool potion_done=false;for(unsigned i=0;i<game.effect_count;++i){const auto&e=game.effects[i];if(e.kind==EffectKind::Extended&&e.target==int(XOp::PotionDone)){potion_done=e.value==int(Potion::ColorlessPotion)&&e.aux==-1;}}
 require(potion_done,"non-targeted potion continuation records no enemy target");persisted();
 const uint16_t offered=MaxCards;require(choose_extended(game,&offered,1),"saved colorless potion offer resolves");
 require(game.phase==Phase::Combat&&!game.expansion.potion_active&&game.strength==3,"manual potion resumes once after its selection");persisted();

 // LoseMaxHp damages against the raw new cap before SetMaxHp. Tungsten
 // modifies that damage; the subsequent cap is not a second damage result.
 start();finish_routing_fixture();require(gain_world_relic(game,WorldRelicTungstenRod)&&gain_world_relic(game,WorldRelicCentennialPuzzle)&&gain_world_relic(game,WorldRelicSelfFormingClay),"obtain maximum-HP damage fixtures");choose_first();
 game.hp=game.max_hp;const int max_before=game.max_hp;player_lose_max_hp(game,5);resolve_pending(game);
 require(game.max_hp==max_before-5&&game.hp==game.max_hp&&game.ironclad.hp_losses_combat==1,"maximum-HP loss awaits one real damage result then applies the cap");
 require(world_relic_counter(game,WorldRelicCentennialPuzzle)==1&&game.world_relics.self_forming_clay==3,"maximum-HP damage notifies received listeners");
 bool reduced_loss=false;for(unsigned i=0;i<game.log_count;++i)if(game.logs[i].kind==LogKind::LoseHp&&game.logs[i].a==4)reduced_loss=true;
 require(reduced_loss,"Tungsten reduces the damage before the new maximum is clamped");persisted();

 // Transform appends after removal. Adjacent original rods each advance once,
 // and their replacements notify permanent-deck listeners including copies.
 start();finish_routing_fixture();const int rod_a=event_add_deck_card(game,{EventCardDowsing,0});const int rod_b=event_add_deck_card(game,{EventCardDowsing,0});
 require(rod_a>=0&&rod_b==rod_a+1,"adjacent Dowsing rods added");game.event_cards.deck[rod_a].progress=game.event_cards.deck[rod_b].progress=4;
 require(gain_world_relic(game,WorldRelicBingBong)&&gain_world_relic(game,WorldRelicLuckyFysh),"obtain transform deck listeners");
 const int transform_gold=game.gold;const unsigned transform_deck=game.deck_count;
 event_card_before_room(game,true,true);unsigned abundance=0,rods=0;for(unsigned i=0;i<game.deck_count;++i){abundance+=game.deck[i].kind==EventCardAbundance;rods+=game.deck[i].kind==EventCardDowsing;}
 require(!rods&&abundance==4&&game.deck_count==transform_deck+2,"both rods transform and each replacement clones exactly once");
 require(game.gold==transform_gold+60,"transformed replacements and copies notify Lucky Fysh");persisted();

 combat_extra_reward_group();
 start();game.hp=0;require(world_finish_room(game)&&game.phase==Phase::Dead&&game.world.finished,"native death closes and records the run");persisted();
 std::printf("original world routing checks: %u\n",checks);return 0;
}
