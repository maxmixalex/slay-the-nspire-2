#include "core.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
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
int main(){
 start();require(game.potion_slots==3,"default run has three potion slots");persisted();
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
 require(world_rest_choose(game,1)&&game.phase==Phase::Upgrade,"native smith opens deck selection");
 require(world_rest_cancel(game)&&game.phase==Phase::Camp&&!game.world.rest_used,"cancelled smith leaves option unused");
 require(gain_world_relic(game,WorldRelicMiniatureTent),"obtain tent fixture");
 game.hp=30;require(world_rest_choose(game,0),"rest heals through native option");
 require(game.phase==Phase::Camp&&(game.world.rest_used&1)&&!world_rest_option_enabled(game,0)&&world_rest_option_enabled(game,1),"tent preserves unused camp options");
 require(gain_world_relic(game,WorldRelicMeatCleaver),"obtain cook fixture");
 game.world.rest_options[2]=uint8_t(WorldRestOption::Cook);game.world.rest_count=3;
 unsigned deck=game.deck_count,maxhp=game.max_hp;
 require(world_rest_choose(game,2)&&game.phase==Phase::WorldRelicChoice,"cook pauses for exact selection");
 uint16_t eligible[MaxDeck];unsigned available=world_relic_eligible(game,eligible,MaxDeck);require(available>=2,"cook has removable cards");
 require(!world_rest_selection_allowed(game,1),"cook refuses a one-card partial operation");
 require(world_relic_select(game,eligible,2),"cook accepts exactly two instances");
 require(game.deck_count==deck-2&&game.max_hp==maxhp+5&&game.phase==Phase::Camp,"cook removes two then gains five maximum HP");

 // Exercise the shared engine through a real first combat, including its
 // complete first draw, enemy side and native reward handoff. The final damage
 // is an explicit fixture command, not a player strategy or enemy-parity claim.
 start();finish_routing_fixture();
 require(gain_world_relic(game,WorldRelicAnchor)&&gain_world_relic(game,WorldRelicVajra)&&gain_world_relic(game,WorldRelicPaelsWing),"obtain shared-engine fixtures before room entry");
 choose_first();require(game.phase==Phase::Combat&&game.turn==1&&game.enemy_world.active,"map enters a native encounter");
 require(game.block==10&&game.strength==1,"first-turn clear retains Anchor and native Vajra runs exactly once");
 require(game.hand.count==5&&game.playing<0&&!game.effect_count,"first draw and native hooks completely resume");persisted();
 require(end_turn(game),"end player side through native continuations");
 require(game.phase==Phase::Combat&&game.turn==2&&!game.expansion.enemy_side&&!game.enemy_world.side&&!game.effect_count,"enemy side completes before the next player turn");persisted();
 for(unsigned guard=0;guard<64&&game.phase==Phase::Combat;guard++){
  for(unsigned i=0;i<game.enemy_count;i++)if(enemy_allow_target(game,int(i)))damage_unpowered(game,int(i),9999);
  resolve_pending(game);
 }
 require(game.phase==Phase::Reward&&game.world.reward_count>=3,"victory generates original rewards after all death hooks");
 require(world_card_reward_alternatives(game)==1,"Pael's Wing is offered for an actual native card reward");
 unsigned old_deck=game.deck_count;require(world_take_card_reward_alternative(game,1),"sacrifice consumes the card reward");
 require((game.reward_claimed&2)&&game.deck_count==old_deck&&world_relic_counter(game,WorldRelicPaelsWing)==1,"sacrifice records one reward without adding a card");
 other=game;require(!world_take_card_reward_alternative(game,1)&&!std::memcmp(&game,&other,sizeof game),"already consumed reward cannot be sacrificed twice");persisted();
 std::printf("original world routing checks: %u\n",checks);return 0;
}
