#include "core.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace spire;
static State game,restored;
static SaveData before,after;
static unsigned checks;
static void check(bool value,const char*message){
    ++checks;if(!value){std::fprintf(stderr,"retained room: %s\n",message);std::exit(1);}
}
static void snapshot(){check(save(game,before),"state serializes before a guarded action");}
static void unchanged(){check(save(game,after)&&before.size==after.size&&
    !std::memcmp(before.bytes,after.bytes,before.size),"guarded action preserves every saved field and RNG word");}
static void roundtrip(){snapshot();check(load(restored,before.bytes,before.size),"retained room loads");
    game=restored;unchanged();}
static void start(){
    new_run(game,0x7195c3,false,Character::Ironclad);
    check(world_set_seed(game,"NSPIRE-WORLD-017"),"set original deterministic seed");
    check(world_begin_run(game),"begin original three-act run");
}
// Room outcomes here isolate the route controller from card strategy. Room
// construction, inventories, map coordinates and the tested actions are real.
static void complete_fixture(){
    game.original_events=OriginalEventState{};
    if(combat_phase(game.phase))game.phase=Phase::Reward;
    check(world_finish_room(game),"fixture supplies completed room outcome");
}
static unsigned first_destination(){
    uint8_t ids[reference::Columns];const auto n=world_available_points(game,ids,reference::Columns);
    check(n>0&&n<=reference::Columns,"retained room has an original frontier");return ids[0];
}
static void seek(WorldRoom wanted){
    start();
    for(unsigned guard=0;guard<60&&!game.world.final_event;++guard){
        if(game.world.room_type==unsigned(wanted)&&game.world.room_open){check(validate(game),"actual target room validates");return;}
        if(!world_can_travel(game))complete_fixture();
        uint8_t ids[reference::Columns];const auto n=world_available_points(game,ids,reference::Columns);
        check(n>0,"routing fixture has a next point");unsigned choice=ids[0];
        const auto*map=world_current_map(game);
        for(unsigned i=0;i<n;++i){const auto type=map->points[ids[i]].type;
            if((wanted==WorldRoom::Shop&&type==reference::PointType::Shop)||
               (wanted==WorldRoom::Rest&&type==reference::PointType::Rest)||
               (wanted==WorldRoom::Treasure&&type==reference::PointType::Treasure)){choice=ids[i];break;}}
        check(world_choose_point(game,choice),"enter a real original map room");
    }
    check(false,"fixture reaches the requested room");
}
static void finished_event_retains_scene(){
    start();check(!world_can_travel(game),"unfinished Neow blocks travel");
    // Supply the event's terminal page; test its real Leave/controller path.
    auto&e=game.original_events;e.finished=1;e.option_count=1;e.options[0]=0;
    e.step_count=e.step_cursor=0;e.selection=OriginalEventSelection::None;
    const auto id=e.id;const auto rooms=game.world.rooms_completed;
    check(world_can_travel(game),"finished event page enables top-bar travel");
    check(original_event_leave(game),"event Proceed opens its route");
    check(game.phase==Phase::Event&&e.active&&e.finished&&e.id==id&&game.world.room_open==2,
        "event background and terminal page stay active beneath the map");
    check(game.world.rooms_completed==rooms+1,"event completion counted once");roundtrip();
    snapshot();check(original_event_leave(game),"event Proceed can reopen its map");unchanged();
    check(!world_choose_point(game,reference::MaxPoints),"bad destination refused");unchanged();
    game.world.room_open=3;check(!validate(game),"unsupported room lifecycle value rejected");game.world.room_open=2;
    const unsigned point=first_destination();check(world_choose_point(game,point),"travel submits the actual next room");
    check(!game.original_events.active&&game.world.room_open==1&&game.world.rooms_completed==rooms+1,
        "actual departure closes the old event without completing it twice");roundtrip();
}
static void merchant_remains_usable(){
    seek(WorldRoom::Shop);game.gold=9999;const unsigned rooms=game.world.rooms_completed;
    check(game.world.room_open==1&&world_can_travel(game),"merchant permits top-bar travel before Proceed");
    const auto offered=game.world.shop_cards[0];const int price=world_shop_price(game,0);
    check(price>=0&&price<=game.gold,"real offered card is affordable");
    check(leave_shop(game)&&game.phase==Phase::Shop&&game.world.room_open==2,"merchant Proceed retains its room");
    roundtrip();snapshot();check(leave_shop(game),"merchant map can reopen");unchanged();
    const int gold=game.gold;const unsigned deck=game.deck_count;
    check(buy(game,0)&&game.phase==Phase::Shop,"closing map allows a real merchant purchase");
    check(game.deck_count==deck+1&&game.deck[deck].kind==offered.kind&&game.gold==gold-price,
        "retained inventory charges and grants exactly once");
    check(game.world.rooms_completed==rooms+1&&world_can_travel(game),"purchase preserves completed route permission");roundtrip();
    const unsigned floor=game.world.total_floor;check(world_choose_point(game,first_destination()),"merchant departure enters next room");
    check(game.world.total_floor==floor+1&&game.world.room_open==1&&game.world.rooms_completed==rooms+1,
        "merchant departure increments only the actual new floor");roundtrip();
}
static void treasure_keeps_awaited_pickup(){
    seek(WorldRoom::Treasure);game.world.treasure_relic=WorldRelicDollysMirror;
    check(!game.treasure_state&&!world_can_travel(game),"unopened treasure keeps map read-only");
    const unsigned rooms=game.world.rooms_completed;snapshot();check(!leave_treasure(game),"unopened chest rejects Proceed");unchanged();roundtrip();
    check(game.phase==Phase::Treasure&&game.world.treasure_relic==WorldRelicDollysMirror&&!game.treasure_state,
        "read-only map viewing retains the unopened chest offer");
    check(open_treasure(game),"returning from read-only map can open retained chest");
    const unsigned deck=game.deck_count,relics=game.world_relics.count;
    check(take_treasure(game)&&game.phase==Phase::WorldRelicChoice,"retained Dolly starts its actual awaited choice");
    check(!world_can_travel(game),"awaited pickup disables travel until selection resolves");roundtrip();
    snapshot();check(!world_choose_point(game,game.world.maps[game.world.act_index].boss),"pending pickup cannot submit another room");unchanged();
    uint16_t eligible[MaxDeck];check(world_relic_eligible(game,eligible,MaxDeck)>0,"Dolly has selectable original cards");
    check(world_relic_select(game,eligible,1),"resolve one actual Dolly selection");
    check(game.phase==Phase::Treasure&&game.treasure_state==2&&game.deck_count==deck+1&&
        game.world_relics.count==relics+1&&world_can_travel(game),"pickup resumes retained chest with one grant");
    snapshot();check(!take_treasure(game),"claimed relic cannot be granted twice");unchanged();roundtrip();
    check(world_choose_point(game,first_destination()),"claimed retained chest can depart");
    check(game.world.rooms_completed==rooms+1&&!game.treasure_state&&!game.treasure_relic,
        "actual departure clears old chest state without recounting completion");roundtrip();
}
static void tent_keeps_optional_choices(){
    seek(WorldRoom::Rest);check(gain_world_relic(game,WorldRelicMiniatureTent),"obtain real Miniature Tent");
    game.hp=30;check(!world_can_travel(game),"unresolved rest site cannot travel");
    check(world_rest_choose(game,0)&&world_can_travel(game)&&game.world.room_open==1,
        "completing Heal enables travel without requesting the map");
    const unsigned rooms=game.world.rooms_completed;const int hp=game.hp;
    check(world_rest_proceed(game)&&game.phase==Phase::Camp&&game.world.room_open==2,
        "rest Proceed retains original Camp phase");roundtrip();
    check(world_rest_option_enabled(game,1)&&world_rest_choose(game,1)&&game.phase==Phase::Upgrade,
        "closing map exposes the Tent's unused Smith option");
    check(!world_can_travel(game),"pending Smith prevents map submission");roundtrip();
    snapshot();check(!world_choose_point(game,reference::MaxPoints),"invalid route during Smith rejected");unchanged();
    check(world_rest_cancel(game)&&world_can_travel(game)&&game.hp==hp&&game.world.room_open==2,
        "cancelling Smith returns to the same completed room");
    check(world_rest_choose(game,1)&&upgrade_card(game,0),"unused Smith can still complete after map return");
    check(game.deck[0].upgraded&&game.phase==Phase::Camp&&game.world.rooms_completed==rooms+1,
        "optional rest choice never repeats completion or healing");roundtrip();
    check(world_choose_point(game,first_destination()),"rest can depart after optional choices");roundtrip();
}
static void direct_map_travel(){
    seek(WorldRoom::Shop);const unsigned rooms=game.world.rooms_completed,floor=game.world.total_floor;
    check(game.world.room_open==1&&world_can_travel(game),"direct map travel starts before explicit Proceed");
    check(world_choose_point(game,first_destination()),"top-bar selection performs actual departure");
    check(game.world.rooms_completed==rooms+1&&game.world.total_floor==floor+1&&game.world.room_open==1,
        "direct selection completes and enters exactly once");roundtrip();
}
static void old_save(const char*path){
    FILE*f=std::fopen(path,"rb");check(f!=nullptr,"open historical frozen-source save");
    before.size=std::fread(before.bytes,1,sizeof before.bytes,f);check(!std::ferror(f)&&std::feof(f),"read complete historical save");std::fclose(f);
    check(before.size>16&&before.bytes[4]==15,"fixture was produced by frozen v15 writer");
    check(load(game,before.bytes,before.size),"new reader accepts old 0/1 lifecycle values");
    unchanged();
}
int main(int argc,char**argv){
    finished_event_retains_scene();merchant_remains_usable();treasure_keeps_awaited_pickup();
    tent_keeps_optional_choices();direct_map_travel();if(argc>1)old_save(argv[1]);
    std::printf("retained room lifecycle: %u checks passed\n",checks);
}
