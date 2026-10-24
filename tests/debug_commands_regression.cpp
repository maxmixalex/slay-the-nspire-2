#include "debug_commands.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if !SPIRE_DEBUG_COMMANDS
#error This regression must only be built with SPIRE_DEBUG_COMMANDS enabled.
#endif

using namespace spire;
static State game,original,loaded;
static SaveData encoded,roundtrip;
static unsigned checks=0;
static void check(bool condition,const char*message){
    ++checks;if(!condition){std::fprintf(stderr,"debug commands: %s (phase %u)\n",message,unsigned(game.phase));std::exit(1);}
}
static void valid(){
    check(validate(game),"result passes the complete gameplay validator");
    check(save(game,encoded)&&load(loaded,encoded.bytes,encoded.size)&&save(loaded,roundtrip),"result save roundtrip");
    check(encoded.size==roundtrip.size&&!std::memcmp(encoded.bytes,roundtrip.bytes,encoded.size),"roundtrip preserves every saved field");
}
static void snapshot(){valid();original=game;}
static void unchanged(){check(!std::memcmp(&original,&game,sizeof game),"rejected command preserves State, choices and RNG byte for byte");}
static void start(Character character=Character::Ironclad){
    new_run(game,0x715e2,false,character);
    check(world_set_seed(game,"NSPIRE-UI6-DEBUG"),"set original seed");
    check(world_begin_run(game),"start canonical run");valid();
}
static void combat(EncounterId encounter=EncounterId::NibbitsWeak){
    check(debug_enter_room(game,WorldRoom::Monster,encounter),"enter requested original encounter");
    check(game.phase==Phase::Combat&&game.enemy_world.encounter==encounter,"requested encounter is actually playable");valid();
}
static void kill_and_reward(){
    const unsigned next_id=game.enemy_world.next_creature_id;
    check(debug_kill_enemies(game),"force-kill succeeds");
    check(game.phase==Phase::Reward&&game.world.reward_count>0,"force-kill runs the actual victory and reward construction");
    check(!game.effect_count&&game.playing==-1&&!game.expansion.depth,"victory owns no orphaned continuation");
    for(unsigned i=0;i<game.enemy_count;++i)check(!game.enemies[i].hp&&(game.enemy_world.models[i].flags&EnemyRemoved),"all original actors are finally removed");
    check(game.enemy_world.next_creature_id==next_id,"force-kill does not leave a replacement actor alive");valid();
}
static void force_deaths(){
    constexpr EncounterId encounters[]={EncounterId::AxebotsNormal,EncounterId::DecimillipedeElite,
        EncounterId::TestSubjectBoss,EncounterId::PhrogParasiteElite,EncounterId::GremlinMercNormal,
        EncounterId::TheObscuraNormal,EncounterId::WaterfallGiantBoss};
    for(auto encounter:encounters){
        start();combat(encounter);
        if(encounter==EncounterId::WaterfallGiantBoss){
            queue_extension(game,XOp::EnemyApplyPower,3,int(EnemyPower::SteamEruption)+80);resolve_pending(game);valid();
        }
        game.hp=int16_t(game.max_hp-10);kill_and_reward();
        check(game.hp>game.max_hp-10,"original combat-victory relic listeners still heal the player");
    }
    start();combat(EncounterId::TestSubjectBoss);
    damage_unpowered(game,0,9999);resolve_pending(game);
    check(game.phase==Phase::Combat&&(game.enemy_world.models[0].flags&EnemyDown),"real first death enters retained reviver state");
    kill_and_reward();
    start();combat(EncounterId::NibbitsNormal);check(game.enemy_count>1,"single target fixture has multiple enemies");
    const int untouched=game.enemies[1].hp;
    check(debug_kill_enemies(game,0)&&game.phase==Phase::Combat,"single-target kill keeps another primary enemy in combat");
    check(!game.enemies[0].hp&&game.enemies[1].hp==untouched,"single-target kill preserves the other actor");valid();
    snapshot();check(!debug_kill_enemies(game,0),"removed actor rejects a second kill");unchanged();
}
static void begin_exhaust_choice(){
    start();
    game.deck_count=3;game.deck[0]={BurningPact,0};game.deck[1]={Defend,0};game.deck[2]={Strike,0};
    combat();int index=-1;
    for(unsigned i=0;i<game.hand.count;++i)if(game.cards[game.hand.ids[i]].kind==BurningPact)index=int(i);
    check(index>=0&&play(game,index,0)&&game.phase==Phase::ExhaustChoice,"real card opens a suspended mandatory choice");valid();
}
static void pending_choices(){
    begin_exhaust_choice();snapshot();check(!debug_kill_enemies(game,0),"single-target command cannot abandon an active player card");unchanged();
    kill_and_reward();
    begin_exhaust_choice();const unsigned deck=game.deck_count;
    check(debug_enter_room(game,WorldRoom::Rest),"room warp cancels a suspended card");
    check(game.phase==Phase::Camp&&game.deck_count==deck&&!game.card_count&&!game.effect_count&&game.playing==-1,"room warp clears combat owners while preserving permanent deck");valid();
    check(world_rest_choose(game,1)&&game.phase==Phase::Upgrade&&game.world.rest_pending,"real rest option awaits its deck choice");
    check(debug_enter_room(game,WorldRoom::Treasure),"treasure warp cancels rest continuation");
    check(!game.world.rest_pending&&!game.world.rest_count,"rest continuation cannot resume inside another room");
    game.world.treasure_relic=WorldRelicDollysMirror;
    check(open_treasure(game)&&take_treasure(game)&&game.phase==Phase::WorldRelicChoice,"real treasure relic opens an awaited choice");valid();
    const unsigned relics=game.world_relics.count;
    check(debug_enter_room(game,WorldRoom::Shop),"merchant warp cancels relic selection");
    check(game.phase==Phase::Shop&&!world_relic_pending(game)&&game.world_relics.count==relics,"obtained relic survives but its old selection cannot resume");valid();
    game.gold=9999;check(buy(game,ShopRemove)&&game.phase==Phase::Remove,"merchant removal opens a real deck selection");
    check(debug_enter_room(game,WorldRoom::Event,EncounterId::None,OriginalEventId::AromaOfChaos),"event warp cancels merchant removal");
    check(game.original_events.active&&game.original_events.id==OriginalEventId::AromaOfChaos&&game.world.shop_pending_item==255&&!game.world.shop_free_remove,"new event owns its complete continuation");valid();
    check(original_event_choose(game,0),"original event option executes");
    check(debug_enter_act(game,2),"act jump cancels original event state");
    check(game.phase==Phase::Map&&game.world.act_index==2&&!game.original_events.active,"act jump reaches the actual third-act map");valid();
}
static void permanent_state_and_routes(){
    for(unsigned character=0;character<CharacterKinds;++character){
        start(Character(character));
        const int genetic=event_add_deck_card(game,{DefectGeneticAlgorithm,0});
        const int scythe=event_add_deck_card(game,{NecroTheScythe,0});
        check(genetic>=0&&scythe>=0,"append permanent growth cards");
        game.defect.genetic_deck[genetic]=31;game.necro.scythe_deck[scythe]=37;
        game.event_cards.deck[0].enchantment=EventEnchantment::Sharp;game.event_cards.deck[0].enchantment_amount=3;
        const unsigned deck=game.deck_count,floor=game.world.total_floor,point=game.world.current_point;
        combat();check(debug_enter_room(game,WorldRoom::Rest),"leave actual combat for rest");
        check(game.defect.genetic_deck[genetic]==31&&game.necro.scythe_deck[scythe]==37&&game.event_cards.deck[0].enchantment==EventEnchantment::Sharp&&game.deck_count==deck,"all permanent growth and enchantments survive room cancellation");
        check(game.world.total_floor==floor&&game.world.current_point==point,"room replacement invents no traveled map floor");
        check(world_rest_choose(game,0)&&world_rest_proceed(game),"real replaced room can finish normally");valid();
        check(debug_enter_room(game,WorldRoom::Rest)&&world_rest_choose(game,0)&&world_rest_proceed(game),"repeated replacement cannot overcount completed rooms");valid();
        check(debug_enter_act(game,2)&&game.world.map_ready[1]&&game.world.map_ready[2],"skipped acts get original generated maps");valid();
        check(debug_enter_room(game,WorldRoom::Rest)&&game.world.current_point==game.world.maps[2].root,"first room at new act records its real Start coordinate once");valid();
        check(debug_enter_act(game,0)&&game.world.act_index==0&&game.world.map_ready[0]&&!game.world.map_ready[1]&&!game.world.map_ready[2],"rewind discards stale later maps and histories");valid();
    }
}
static void guards_and_rollback(){
    start();snapshot();check(!debug_enter_room(game,WorldRoom::None),"None room rejected");unchanged();
    check(!debug_enter_room(game,WorldRoom(255)),"invalid room rejected");unchanged();
    check(!debug_enter_room(game,WorldRoom::Monster,EncounterId::Count),"invalid encounter rejected");unchanged();
    check(!debug_enter_room(game,WorldRoom::Event,EncounterId::None,OriginalEventId::Count),"invalid event rejected");unchanged();
    check(!debug_enter_room(game,WorldRoom::Rest,EncounterId::NibbitsWeak),"incompatible encounter argument rejected");unchanged();
    check(!debug_enter_act(game,3)&&!debug_kill_enemies(game)&&!debug_kill_enemies(game,-2),"invalid act and noncombat kill rejected");unchanged();
    game.world.combat_count=255;snapshot();
    check(!debug_enter_room(game,WorldRoom::Monster,EncounterId::NibbitsWeak),"bounded combat counter rejects transaction after cleanup");unchanged();
    game.world.combat_count=0;combat();
    player_kill(game,true);resolve_pending(game);check(game.phase==Phase::Dead,"real death fixture");valid();
    check(debug_enter_room(game,WorldRoom::Rest)&&game.hp==game.max_hp&&!game.world.finished,"debug warp can revive a terminated run");valid();
    check(debug_enter_room(game,WorldRoom::Map)&&game.phase==Phase::Map&&!game.world.room_open,"map command leaves a valid playable map");valid();
}
int main(){
    force_deaths();pending_choices();permanent_state_and_routes();guards_and_rollback();
    std::printf("debug commands regression: %u checks passed\n",checks);return 0;
}
