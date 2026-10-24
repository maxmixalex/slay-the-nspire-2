#include "app.hpp"
#include "original_events.hpp"
#include "world_relics.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace spire;
static App app;
static State departure,oracle,replacement;
static SaveData before,after,expected,disk;
static unsigned checks,reads,writes;
static bool disk_exists,read_ok=true,write_ok=true;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"map travel UI: %s:%d %s (phase=%u, clock=%u)\n",__FILE__,__LINE__,#x,unsigned(app.game.phase),app.map_clock_ms);std::exit(1);}}while(0)

static void put(std::vector<uint8_t>&b,unsigned at,unsigned value,unsigned count=4){
    for(unsigned i=0;i<count;++i)b[at+i]=uint8_t(value>>(i*8));
}
static std::vector<uint8_t> synthetic_profile(){
    // Complete current directories, shared tiny payload, and no original art.
    // Stable dimensions preserve the production map's real hit geometry.
    constexpr unsigned images=CardKinds>U_Count?CardKinds:U_Count;
    constexpr unsigned stringdir=40+images*16,clipdir=stringdir+ContentStrings*8;
    constexpr unsigned carddir=clipdir+AnimationKinds*16,uidir=carddir+CardKinds*16;
    constexpr unsigned payload=uidir+U_Count*4;
    std::vector<uint8_t>b(payload+82*60*2);
    put(b,0,0x3250534e);put(b,4,ContentVersion);put(b,8,0x41cef1ea);
    put(b,16,images);put(b,24,ContentStrings);put(b,28,AnimationKinds);put(b,32,CardKinds);
    for(unsigned i=0;i<images;++i){
        put(b,40+i*16,payload);put(b,44+i*16,82,2);put(b,46+i*16,60,2);put(b,52+i*16,82*60*2);
    }
    for(unsigned i=0;i<82*60;++i)put(b,payload+i*2,0xc53a,2);
    const char*text="SYNTHETIC_MAP_TRAVEL";
    for(unsigned i=0;i<ContentStrings;++i){
        put(b,stringdir+i*8,unsigned(b.size()));put(b,stringdir+i*8+4,unsigned(std::strlen(text)));
        b.insert(b.end(),text,text+std::strlen(text)+1);
    }
    for(unsigned i=0;i<AnimationKinds;++i){put(b,clipdir+i*16+4,1,2);put(b,clipdir+i*16+6,33,2);}
    for(unsigned i=0;i<CardKinds;++i)for(unsigned j=0;j<4;++j)put(b,carddir+i*16+j*4,j?i*3+j-1:i);
    for(unsigned i=0;i<U_Count;++i)put(b,uidir+i*4,i==U_CampFlames?A_Count:i);
    put(b,12,unsigned(b.size()));uint32_t hash=2166136261u;
    for(unsigned i=40;i<b.size();++i){hash^=b[i];hash*=16777619u;}put(b,36,hash);return b;
}
static bool store(const State&s){
    ++writes;if(!write_ok||!save(s,disk))return false;disk_exists=true;return true;
}
static bool restore(State&s){
    ++reads;
    // A failed storage callback can partially modify its output. App must
    // isolate this probe from both the visible run and its pending route.
    if(!read_ok){s.gold=9998;s.hp=1;return false;}
    return disk_exists&&load(s,disk.bytes,disk.size);
}
static void same_bytes(const SaveData&a,const SaveData&b){
    CHECK(a.size==b.size);CHECK(!std::memcmp(a.bytes,b.bytes,a.size));
}
static void snapshot(){CHECK(validate(app.game));CHECK(save(app.game,before));}
static void same_state(const SaveData&bytes){
    CHECK(validate(app.game));CHECK(save(app.game,after));same_bytes(bytes,after);
}
static void advance_ms(unsigned ms){
    while(ms){const unsigned step=ms>173?173:ms;app.advance(step);ms-=step;}
}
static void finish_routing_fixture(){
    // As in the map UI suite, explicitly supply completed room outcomes.
    // The generated graph, room-entry code, saves and RNG remain real; this
    // fixture does not claim to play combat or solve each event's choices.
    if(app.game.original_events.active){
        if(app.game.world.event_combat_parent){
            app.game.world.room_type=app.game.world.parent_room_type;
            app.game.world.parent_room_type=app.game.world.event_combat_parent=0;
        }
        app.game.original_events=OriginalEventState{};app.game.phase=Phase::Event;
    }
    if(world_room_is_combat(app.game))app.game.phase=Phase::Reward;
    if(app.game.phase==Phase::Treasure)app.game.treasure_state=2;
    CHECK(world_finish_room(app.game));CHECK(validate(app.game));
    CHECK(world_can_travel(app.game));app.reset_presentation();
    // This isolated route fixture starts after act presentation has settled.
    app.reset_map_act_intro(true);app.open_map_window();if(app.map_window.is_open)app.map_window.settle();
}
static void new_map(reference::ActKind region){
    new_run(app.game,0x175c3,false,Character::Silent);
    CHECK(world_set_seed(app.game,"MAP-TRAVEL-14"));CHECK(world_begin_run(app.game,region,0));
    finish_routing_fixture();
}
static void fresh(bool fast=false,uint32_t clock=1000){
    app.game=departure;app.title=app.paused=app.quit=app.help=app.library=app.character_select=false;
    app.settings_open=app.run_settings=app.run_seed_editing=false;app.frontend_confirm=app.frontend_choice=0;
    app.map_preview.open=false;app.fast_mode=fast;app.map_directional=false;
    app.map_clock_ms=clock;app.animation=app.tick_remainder=0;app.reset_presentation();app.reset_map_act_intro(true);app.open_map_window();if(app.map_window.is_open)app.map_window.settle();
    app.pointer.visible=false;app.pointer.dispatching=false;app.notice="";app.notice_ticks=0;
    app.forget_run_checkpoint();app.has_save=false;
    disk_exists=false;read_ok=write_ok=true;reads=writes=0;
    CHECK(world_can_travel(app.game)&&app.map_routes_active()&&validate(app.game));
}
static unsigned first_point(){
    uint8_t ids[reference::Columns]{};
    const unsigned n=world_available_points(app.game,ids,reference::Columns);CHECK(n&&n<=reference::Columns);
    CHECK(world_reachable(app.game,ids[0]));return ids[0];
}
static HitRegion hit(HitKind kind,int value){
    app.render();const HitRegion*found=nullptr;
    for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==kind&&app.hits[i].value==value&&app.hits[i].enabled){
        CHECK(!found);found=&app.hits[i];
    }
    CHECK(found&&found->w>0&&found->h>0);return *found;
}
static void click(const HitRegion&h){
    app.pointer_move(h.x+h.w/2,h.y+h.h/2);app.pointer_button(true);app.pointer_button(false);
}
static void pointer_begin(unsigned destination){
    const HitRegion node=hit(HitKind::WorldNode,int(destination));
    app.pointer_move(node.x+node.w/2,node.y+node.h/2);
    app.pointer_button(false);CHECK(!app.map_travel.pending); // release alone is not a click
    app.pointer_button(true);CHECK(app.pointer.held&&!app.map_travel.pending);
    app.pointer_button(false);CHECK(app.map_travel.pending&&!app.pointer.held);
    CHECK(app.map_travel.to==destination);
}
static void keyboard_begin(unsigned destination){
    // Select a real reachable column through the ordinary directional input
    // route, then submit through Confirm rather than calling begin directly.
    app.cursor=-1;app.input(Key::Right);
    const auto*m=world_current_map(app.game);CHECK(m&&destination<m->count);
    for(unsigned i=0;i<reference::Columns&&app.cursor!=m->points[destination].col;++i)app.input(Key::Right);
    CHECK(app.cursor==m->points[destination].col&&app.map_directional);
    app.input(Key::Confirm);CHECK(app.map_travel.pending&&app.map_travel.to==destination);
}
static void make_oracle(unsigned destination){
    oracle=app.game;CHECK(world_choose_point(oracle,destination));CHECK(validate(oracle));CHECK(save(oracle,expected));
}
static void assert_cancelled(){
    CHECK(!app.map_travel.pending&&!app.map_travel.fading_in&&!app.map_visuals.traveling());
    CHECK(!app.map_transition_blocks_keyboard()&&!app.map_transition_blocks_pointer());
    CHECK(!app.map_transition_alpha(0)&&!app.map_transition_alpha(ScreenH-1));
}
static void delayed_commit(bool fast,bool pointer,bool toggle,uint32_t start){
    fresh(fast,start);const unsigned destination=first_point(),from=app.game.world.current_point;
    const unsigned floor=app.game.world.total_floor,combats=app.game.world.combat_count;
    const uint32_t duration=fast?600u:1100u;const auto speed=fast?world_map_transition::Speed::Fast:world_map_transition::Speed::Normal;
    const HitRegion stale_node=hit(HitKind::WorldNode,int(destination));snapshot();make_oracle(destination);
    if(pointer)pointer_begin(destination);else keyboard_begin(destination);
    CHECK(app.map_travel.start==start&&app.map_travel.speed==speed);
    CHECK(app.map_visuals.traveling()&&app.map_visuals.target()==destination);
    CHECK(app.map_travel.from==from&&app.map_travel.seed==app.game.world.run_seed);
    CHECK(app.map_travel.act==app.game.world.act_index&&app.map_travel.revision==app.game.revision);same_state(before);
    for(unsigned i=0;i<3;++i){app.input(Key::Confirm);click(stale_node);}
    app.input(Key::Back);
    CHECK(!app.paused&&app.map_scroll==0&&app.map_travel.start==start&&app.map_travel.to==destination);
    CHECK(!app.begin_map_travel(destination));same_state(before);
    // The host delta cap and the selected speed are independent. Change the
    // real Settings control through the HUD during the already begun fade.
    app.advance(100000);CHECK(uint32_t(app.map_clock_ms-start)==250&&app.map_travel.pending);
    if(toggle){
        // The original top-bar Settings button directly opens the pause
        // menu over the map; it is not the map's close/back control.
        click(hit(HitKind::Command,int(Key::Back)));CHECK(app.paused&&app.map_window.is_open);
        click(hit(HitKind::Menu,1));CHECK(app.settings_open);
        click(hit(HitKind::UiAction,int(UiAction::SettingsFastMode)));
        CHECK(app.fast_mode!=fast&&app.map_travel.pending);
    }
    advance_ms(duration-251);CHECK(uint32_t(app.map_clock_ms-start)==duration-1);
    CHECK(app.map_travel.pending&&app.game.world.current_point==from&&app.game.world.room_open==2);
    CHECK(app.map_travel.speed==speed);app.render();same_state(before);
    app.advance(1);
    CHECK(!app.map_travel.pending&&app.map_travel.fading_in&&app.map_travel.speed==speed);
    CHECK(app.map_travel.fade_start==app.map_clock_ms&&!app.map_visuals.traveling());
    CHECK(!app.paused&&!app.settings_open);
    CHECK(app.game.phase==Phase::Combat&&app.game.world.current_point==destination);
    CHECK(app.game.world.total_floor==floor+1&&app.game.world.combat_count==combats+1);
    CHECK(app.choice_revision==app.game.revision);same_state(expected);
    if(start>0xfffffc00u)CHECK(app.map_clock_ms<start);
    CHECK(!app.begin_map_travel(destination));
    for(unsigned i=0;i<4;++i)app.update_map_travel();
    advance_ms(900);app.render();same_state(expected);
    CHECK(!app.map_travel.pending&&!app.map_travel.fading_in&&reads==0&&writes==0);
}
static void invalid_and_scrolled_input(){
    fresh();snapshot();const unsigned destination=first_point();
    CHECK(!app.begin_map_travel(reference::NoPoint));CHECK(!app.begin_map_travel(app.game.world.current_point));
    CHECK(!app.begin_map_column(-1)&&!app.begin_map_column(reference::Columns));same_state(before);
    app.cursor=world_current_map(app.game)->points[destination].col;app.map_scroll=2;
    app.input(Key::Confirm);CHECK(!app.map_travel.pending&&app.map_scroll==0);same_state(before);
    app.advance(67); // Select changes the target; the source camera lerps back.
    app.input(Key::Confirm);CHECK(app.map_travel.pending);app.cancel_map_travel();assert_cancelled();same_state(before);
    // A different canonical revision between real press and release rejects
    // the stale click without inventing a pending destination.
    const HitRegion node=hit(HitKind::WorldNode,int(destination));
    app.pointer_move(node.x+node.w/2,node.y+node.h/2);app.pointer_button(true);
    CHECK(app.pointer.held);++app.game.revision;snapshot();app.pointer_button(false);
    CHECK(!app.map_travel.pending&&!app.pointer.held);same_state(before);
}
static void persistence_during_travel(){
    fresh();snapshot();const unsigned destination=first_point();keyboard_begin(destination);advance_ms(300);
    app.input(Key::Save);CHECK(writes==1&&disk_exists&&app.map_travel.pending);same_bytes(disk,before);same_state(before);
    write_ok=false;app.input(Key::Save);CHECK(writes==2&&app.map_travel.pending);same_bytes(disk,before);same_state(before);
    read_ok=false;app.input(Key::Load);CHECK(reads==1&&app.map_travel.pending);same_state(before);
    read_ok=write_ok=true;app.input(Key::Load);CHECK(reads==2);assert_cancelled();same_state(before);
    advance_ms(1600);same_state(before);CHECK(app.game.world.current_point==departure.world.current_point);

    fresh(true);snapshot();replacement=app.game;replacement.gold+=7;CHECK(save(replacement,disk));disk_exists=true;
    pointer_begin(first_point());app.input(Key::Load);CHECK(reads==1);assert_cancelled();same_state(disk);
    advance_ms(1000);same_state(disk);CHECK(app.game.gold==departure.gold+7);
}
static void lifecycle_cancellation(){
    for(unsigned kind=0;kind<4;++kind){
        fresh();snapshot();keyboard_begin(first_point());advance_ms(400);
        if(kind==0)app.reset_presentation();
        else if(kind==1)app.reset_presentation(true);
        else if(kind==2)app.return_to_title();
        else app.input(Key::Quit);
        assert_cancelled();CHECK(kind!=2||app.title);CHECK(kind!=3||app.quit);
        advance_ms(1400);same_state(before);
    }
}
static void stale_source_guards(){
    for(unsigned kind=0;kind<5;++kind){
        fresh();snapshot();keyboard_begin(first_point());
        // Keep a fully valid run and corrupt only the remembered departure
        // identity, as if an old asynchronous presentation had survived.
        if(kind==0){app.map_travel.seed^=uint64_t(1)<<32;CHECK(uint32_t(app.map_travel.seed)==uint32_t(app.game.world.run_seed));}
        else if(kind==1)app.map_travel.act=uint8_t(app.game.world.act_index+1);
        else if(kind==2)app.map_travel.from=reference::NoPoint;
        else if(kind==3)++app.map_travel.revision;
        else app.map_travel.to=app.game.world.current_point;
        app.advance(1);assert_cancelled();advance_ms(1300);same_state(before);
    }
    for(unsigned kind=0;kind<2;++kind){
        fresh();const unsigned destination=first_point();keyboard_begin(destination);const uint32_t revision=app.game.revision;
        // Legal HP/gold progress does not change route availability. Travel
        // observes its real source conditions without encoding the whole save.
        if(kind==0)--app.game.hp;else ++app.game.gold;
        CHECK(app.game.revision==revision);snapshot();make_oracle(destination);advance_ms(1099);
        CHECK(app.map_travel.pending&&app.game.revision==revision);same_state(before);
        app.advance(1);CHECK(!app.map_travel.pending&&app.map_travel.fading_in);
        CHECK(app.game.world.current_point==destination);same_state(expected);
        advance_ms(1200);same_state(expected);
    }
}
static void pending_hud_pointer_remains_available(){
    const Key commands[]={Key::Inspect,Key::Back,Key::Map};
    for(unsigned kind=0;kind<3;++kind){
        fresh();const unsigned destination=first_point();snapshot();make_oracle(destination);
        keyboard_begin(destination);advance_ms(400);
        CHECK(app.map_transition_blocks_keyboard()&&!app.map_transition_blocks_pointer());
        click(hit(HitKind::Command,int(commands[kind])));
        if(kind==0)CHECK(app.panel==1);
        else if(kind==1)CHECK(app.map_window.is_open&&app.panel==3&&app.paused);
        else CHECK(!app.map_window.is_open&&!app.panel&&!app.paused);
        app.update_map_travel();CHECK(app.map_travel.pending);same_state(before);
        // The original HUD retains MouseFilter=Ignore during map selection.
        // Its ordinary pointer panels do not abandon the controller's route;
        // keyboard/controller Back still waits for RoomFadeIn's callback.
        app.input(Key::Back);
        if(kind==0)CHECK(app.panel==1);
        else if(kind==1)CHECK(app.map_window.is_open&&app.panel==3&&app.paused);
        else CHECK(!app.map_window.is_open&&!app.panel&&!app.paused);
        advance_ms(699);CHECK(app.map_travel.pending);same_state(before);
        app.advance(1);CHECK(!app.map_travel.pending&&app.map_travel.fading_in);
        CHECK(!app.paused&&!app.panel&&!app.settings_open&&!app.library);
        CHECK(app.game.world.current_point==destination);same_state(expected);
    }
}
static void pending_exit_confirmation(){
    for(unsigned accept=0;accept<2;++accept){
        fresh();const unsigned destination=first_point();snapshot();make_oracle(destination);
        keyboard_begin(destination);advance_ms(250);
        click(hit(HitKind::Command,int(Key::Back)));CHECK(app.paused&&app.map_window.is_open);
        app.input(Key::Quit);CHECK(app.frontend_confirm==3&&app.map_travel.pending&&!app.quit);
        click(hit(HitKind::UiAction,int(accept?UiAction::FrontendAccept:UiAction::FrontendCancel)));
        CHECK(!app.frontend_confirm);
        if(accept){CHECK(app.quit);assert_cancelled();advance_ms(1300);same_state(before);}
        else{
            CHECK(!app.quit&&app.map_travel.pending);same_state(before);
            advance_ms(850);CHECK(!app.map_travel.pending&&!app.paused);same_state(expected);
        }
    }
}
static void pending_compendium_quit_cancels_immediately(){
    fresh();snapshot();keyboard_begin(first_point());advance_ms(250);
    click(hit(HitKind::Command,int(Key::Back)));CHECK(app.paused&&app.map_window.is_open&&app.map_travel.pending);
    click(hit(HitKind::Menu,2));CHECK(app.compendium_open&&!app.library&&app.map_travel.pending);
    click(hit(HitKind::UiAction,int(UiAction::LibraryCards)));CHECK(app.library&&!app.compendium_open&&app.map_travel.pending);
    const uint32_t clock=app.map_clock_ms;
    // This is the card library's real Quit dispatch, without a later update
    // rescuing a stale pending route through the ordinary quit-state guard.
    app.input(Key::Quit);CHECK(app.quit&&app.map_clock_ms==clock);
    assert_cancelled();same_state(before);advance_ms(1300);same_state(before);
}
static bool route_to_shop(const reference::ReferenceMap&map,unsigned point,unsigned depth=0){
    if(point>=map.count||depth>reference::MaxRows+2)return false;
    if(map.points[point].type==reference::PointType::Shop)return true;
    const auto&children=map.points[point].children;
    for(unsigned i=0;i<children.used;++i)if(route_to_shop(map,children.slots[i],depth+1))return true;
    return false;
}
static void fast_shop_can_immediately_start_pointer_travel(){
    // Rest options now have their original 500ms entrance input block, so
    // they cannot exercise this separate 300ms fade-in channel contract.
    // The shop's closed-room Proceed is genuinely available immediately.
    fresh(true);unsigned shop=reference::NoPoint;
    for(unsigned step=0;step<reference::MaxRows&&shop==reference::NoPoint;++step){
        const auto*m=world_current_map(app.game);CHECK(m&&app.game.world.act_index==0);
        uint8_t ids[reference::Columns]{};const unsigned count=world_available_points(app.game,ids,reference::Columns);CHECK(count);
        unsigned next=reference::NoPoint;
        for(unsigned i=0;i<count;++i)if(route_to_shop(*m,ids[i])){next=ids[i];break;}
        CHECK(next!=reference::NoPoint);
        if(m->points[next].type==reference::PointType::Shop){shop=next;break;}
        CHECK(world_choose_point(app.game,next));finish_routing_fixture();
    }
    CHECK(shop!=reference::NoPoint);
    snapshot();make_oracle(shop);keyboard_begin(shop);advance_ms(600);same_state(expected);
    CHECK(app.game.phase==Phase::Shop&&!app.map_travel.pending&&app.map_travel.fading_in&&!app.shop_open);
    // The actual Proceed starts offscreen. Its first moving portion is
    // clickable while Fast's independent 300ms keyboard gate is still held.
    app.render();app.advance(33);app.render();
    const uint32_t clock=app.map_clock_ms;const int hp=app.game.hp,gold=app.game.gold;
    click(hit(HitKind::UiAction,int(UiAction::LeaveShop)));
    CHECK(app.game.phase==Phase::Shop&&app.game.world.room_open==2&&app.map_window.is_open&&app.game.world.current_point==shop&&app.map_clock_ms==clock);
    CHECK(app.game.hp==hp&&app.game.gold==gold);
    CHECK(app.map_travel.fading_in&&app.map_transition_alpha(100)==world_map_transition::fade_in_alpha(world_map_transition::Speed::Fast,33));
    CHECK(app.map_transition_blocks_keyboard()&&!app.map_transition_blocks_pointer());
    const unsigned next=first_point();snapshot();
    // Give Confirm a valid destination so its rejection tests the remaining
    // remaining keyboard gate. The following pointer selects the real node hit.
    app.cursor=world_current_map(app.game)->points[next].col;app.map_scroll=0;
    app.input(Key::Confirm);CHECK(!app.map_travel.pending&&app.map_clock_ms==clock);same_state(before);
    make_oracle(next);pointer_begin(next);
    CHECK(app.map_travel.start==clock&&!app.map_travel.fading_in);same_state(before);
    advance_ms(599);CHECK(app.map_travel.pending);same_state(before);
    app.advance(1);CHECK(!app.map_travel.pending&&app.game.world.current_point==next);same_state(expected);
}
static void fade_in_input_channels(bool fast){
    fresh(fast);const unsigned destination=first_point();
    // Capture a real HUD target before Normal fade-in deliberately clears
    // hit regions. Reusing its coordinates exercises the input gate itself.
    const HitRegion map_button=hit(HitKind::Command,int(Key::Map));
    make_oracle(destination);keyboard_begin(destination);
    advance_ms(fast?600:1100);CHECK(app.map_travel.fading_in&&app.map_transition_alpha(100)==255);
    app.input(Key::Map);CHECK(app.panel==0&&app.map_transition_blocks_keyboard());
    if(fast){
        CHECK(!app.map_transition_blocks_pointer());click(map_button);CHECK(app.panel==3);
        app.input(Key::Map);CHECK(app.panel==3);click(map_button);CHECK(app.panel==0);
        advance_ms(299);CHECK(app.map_transition_blocks_keyboard()&&app.map_travel.fading_in);
        app.input(Key::Map);CHECK(app.panel==0);app.advance(1);
        CHECK(!app.map_transition_blocks_keyboard()&&!app.map_travel.fading_in&&!app.map_transition_alpha(100));
    }else{
        CHECK(app.map_transition_blocks_pointer());click(map_button);CHECK(app.panel==0);
        advance_ms(199);app.input(Key::Map);click(map_button);CHECK(app.panel==0);
        CHECK(app.map_transition_blocks_keyboard()&&app.map_transition_blocks_pointer());app.advance(1);
        CHECK(!app.map_transition_blocks_keyboard()&&!app.map_transition_blocks_pointer());
        CHECK(app.map_travel.fading_in&&app.map_transition_alpha(100)>0);
    }
    if(fast){CHECK(app.map_window.visible&&!app.map_window.is_open);app.input(Key::Map);CHECK(!app.panel);advance_ms(50);CHECK(!app.map_window.visible);}
    app.input(Key::Map);CHECK(app.panel==3);click(map_button);CHECK(app.panel==0);same_state(expected);
    if(!fast){advance_ms(600);CHECK(!app.map_travel.fading_in&&!app.map_transition_alpha(100));same_state(expected);}
}
static void free_travel_commits_once(){
    fresh(true);CHECK(gain_world_relic(app.game,WorldRelicWingedBoots));
    unsigned destination=reference::NoPoint;
    for(unsigned step=0;step<reference::MaxRows&&destination==reference::NoPoint;++step){
        const auto*m=world_current_map(app.game);CHECK(m&&app.game.world.current_point<m->count);
        const auto&from=m->points[app.game.world.current_point];
        for(unsigned i=0;i<m->count;++i)if(m->points[i].active&&m->points[i].row==from.row+1&&
            !from.children.contains(uint8_t(i))&&world_reachable(app.game,i)){destination=i;break;}
        if(destination!=reference::NoPoint)break;
        const unsigned next=first_point();CHECK(from.children.contains(uint8_t(next)));
        CHECK(world_choose_point(app.game,next));finish_routing_fixture();
    }
    CHECK(destination!=reference::NoPoint&&world_relic_counter(app.game,WorldRelicWingedBoots)==3);
    CHECK(!world_map_path_dot_count(app.game,app.game.world.current_point,destination));
    snapshot();pointer_begin(destination);CHECK(!app.map_visuals.travel_path_ms);advance_ms(250);
    CHECK(world_relic_counter(app.game,WorldRelicWingedBoots)==3);app.cancel_map_travel();assert_cancelled();same_state(before);
    make_oracle(destination);pointer_begin(destination);advance_ms(599);
    CHECK(app.map_travel.pending&&world_relic_counter(app.game,WorldRelicWingedBoots)==3);same_state(before);
    app.advance(1);CHECK(!app.map_travel.pending&&world_relic_counter(app.game,WorldRelicWingedBoots)==2);same_state(expected);
    for(unsigned i=0;i<4;++i)app.update_map_travel();advance_ms(900);
    CHECK(world_relic_counter(app.game,WorldRelicWingedBoots)==2);same_state(expected);
}
static void preparation_preserves_room_fade(bool fast){
    fresh(fast);const unsigned destination=first_point();make_oracle(destination);keyboard_begin(destination);
    advance_ms(fast?600:1100);CHECK(app.game.phase==Phase::Combat&&app.map_travel.fading_in);same_state(expected);
    app.sync_combat_preparation();auto&preparation=app.combat_preparation;
    // Hold the public decoder before its first displayed frame. This models
    // a slow host resource callback without inventing canonical combat work.
    preparation.active=true;preparation.shown=false;
    const uint32_t animation=app.animation;
    for(unsigned i=0;i<4;++i){app.advance(250);CHECK(preparation.active&&app.animation==animation);
        CHECK(app.map_travel.fade_start==app.map_clock_ms&&app.map_transition_alpha(100)==255);same_state(expected);}
    app.pointer.visible=false;app.render();CHECK(preparation.shown);
    for(uint16_t pixel:app.pixels)CHECK(pixel==0); // no synthetic loading art/text
    app.advance(33);CHECK(!preparation.active&&app.map_travel.fade_start==app.map_clock_ms);
    app.render();CHECK(app.map_transition_alpha(100)==255);same_state(expected);
    advance_ms(100);CHECK(app.map_travel.fading_in&&app.map_transition_alpha(100)>0&&app.map_transition_alpha(100)<255);
    advance_ms(fast?200:700);CHECK(!app.map_travel.fading_in&&!app.map_transition_alpha(100));same_state(expected);
}
static void entry_observer_survives_reset(){
    fresh();new_map(reference::ActKind::Underdocks);
    // A supported member of this actual region's boss pool supplies an
    // observable entry action. Keep the entire generated map and route.
    app.game.world.rooms[0].bosses[0]=EncounterId::LagavulinMatriarchBoss;
    unsigned destination=reference::NoPoint;
    for(unsigned step=0;step<reference::MaxRows;++step){
        const auto*m=world_current_map(app.game);CHECK(m&&app.game.world.act_index==0);
        if(world_reachable(app.game,m->boss)){destination=m->boss;break;}
        CHECK(world_choose_point(app.game,first_point()));finish_routing_fixture();
    }
    CHECK(destination!=reference::NoPoint);
    // The three real first-hand relic hooks deal 3+9+1 damage, exceeding
    // initial 12 Plating and waking the boss through notify_enemy_animation.
    CHECK(gain_world_relic(app.game,WorldRelicMercuryHourglass));
    CHECK(gain_world_relic(app.game,WorldRelicFestivePopper));
    CHECK(gain_world_relic(app.game,WorldRelicMrStruggles));
    snapshot();make_oracle(destination);keyboard_begin(destination);
    app.enemy_presentation[0].overflow=true;
    app.enemy_presentation[0].count=1;app.enemy_presentation[0].queue[0].action="OLD_ROOM_COMMAND";
    advance_ms(1099);same_state(before);app.advance(1);same_state(expected);
    CHECK(app.game.enemy_world.encounter==EncounterId::LagavulinMatriarchBoss&&app.game.enemy_count==1);
    CHECK(!enemy_power_value(app.game,0,EnemyPower::Asleep));
    int clip=-1;uint32_t elapsed=0;bool loop=true;CHECK(app.enemy_frame(0,clip,elapsed,loop));
    const auto*row=world_enemy_clip_directory(unsigned(clip));CHECK(row&&!std::strcmp(row->native_clip,"wake_up"));
    CHECK(!loop&&!elapsed&&!app.enemy_presentation[0].overflow);
    CHECK(app.enemy_presentation[0].identity==app.game.enemy_world.models[0].combat_id);
    CHECK(app.enemy_visual_clock>app.animation);
    for(unsigned i=0;i<app.enemy_presentation[0].count;++i)
        CHECK(!app.enemy_presentation[0].queue[i].action||std::strcmp(app.enemy_presentation[0].queue[i].action,"OLD_ROOM_COMMAND"));
}
static void entry_damage_initializes_new_enemy_feedback(){
    fresh();auto&q=app.game.world.rooms[app.game.world.act_index];CHECK(q.encounter_count);
    // Keep a genuine generated Monster point, using a valid weak encounter
    // from its original region's pool to make the observed entry clip stable.
    q.encounters[q.encounter_cursor%q.encounter_count]=EncounterId::NibbitsWeak;
    CHECK(gain_world_relic(app.game,WorldRelicMercuryHourglass));
    const unsigned destination=first_point();snapshot();make_oracle(destination);keyboard_begin(destination);
    advance_ms(1099);same_state(before);app.advance(1);same_state(expected);
    CHECK(app.game.enemy_count==1&&app.game.enemy_world.models[0].kind==EnemyId::Nibbit);
    CHECK(app.game.enemies[0].hp<app.game.enemies[0].max_hp);
    int clip=-1;uint32_t elapsed=0;bool loop=true;CHECK(app.enemy_frame(0,clip,elapsed,loop));
    const auto*row=world_enemy_clip_directory(unsigned(clip));CHECK(row&&!std::strcmp(row->native_clip,"hurt"));
    CHECK(!loop&&!elapsed&&!app.enemy_presentation[0].overflow);
    CHECK(app.enemy_presentation[0].identity==app.game.enemy_world.models[0].combat_id);
}
int main(){
    const auto bytes=synthetic_profile();CHECK(app.init(bytes.data(),bytes.size(),{store,restore}));
    new_map(reference::ActKind::Overgrowth);departure=app.game;
    delayed_commit(false,false,false,1000);delayed_commit(true,true,false,2000);
    delayed_commit(false,true,true,3000);delayed_commit(true,false,true,4000);
    delayed_commit(false,false,false,0xfffffe00u);delayed_commit(true,true,false,0xfffffe00u);
    invalid_and_scrolled_input();persistence_during_travel();lifecycle_cancellation();stale_source_guards();
    pending_hud_pointer_remains_available();pending_exit_confirmation();pending_compendium_quit_cancels_immediately();
    fast_shop_can_immediately_start_pointer_travel();fade_in_input_channels(false);fade_in_input_channels(true);
    free_travel_commits_once();entry_observer_survives_reset();entry_damage_initializes_new_enemy_feedback();preparation_preserves_room_fade(false);preparation_preserves_room_fade(true);
    std::printf("map travel UI: %u checks passed; synthetic current profile; canonical save/RNG oracle\n",checks);
}
