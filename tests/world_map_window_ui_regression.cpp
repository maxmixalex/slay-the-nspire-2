#include "app.hpp"
#include "content_fixture.hpp"
#include "original_events.hpp"
#include "world_relics.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace spire;
static App app;
static SaveData before,after,disk;
static State oracle;
static unsigned checks;
static bool actual_profile;
static const char* screenshots;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"map window UI: %s:%d %s (phase=%u, room=%u, open=%u, clock=%u)\n",__FILE__,__LINE__,#x,unsigned(app.game.phase),app.game.world.room_type,app.game.world.room_open,app.map_clock_ms);std::exit(1);}}while(0)
static void put(std::vector<uint8_t>&b,unsigned at,unsigned value,unsigned count=4){for(unsigned i=0;i<count;++i)b[at+i]=uint8_t(value>>(i*8));}
static std::vector<uint8_t> synthetic_profile(){
    // Every current directory is populated. The opaque tiny payload makes
    // an accidental unmodulated map rectangle observable over the live room.
    constexpr unsigned images=CardKinds>U_Count?CardKinds:U_Count;
    constexpr unsigned stringdir=40+images*16,clipdir=stringdir+ContentStrings*8;
    constexpr unsigned carddir=clipdir+AnimationKinds*16,uidir=carddir+CardKinds*16,payload=uidir+U_Count*4;
    std::vector<uint8_t>b(payload+82*60*2);
    put(b,0,0x3250534e);put(b,4,ContentVersion);put(b,8,0x41cef1ea);
    put(b,16,images);put(b,24,ContentStrings);put(b,28,AnimationKinds);put(b,32,CardKinds);
    for(unsigned i=0;i<images;++i){put(b,40+i*16,payload);put(b,44+i*16,82,2);put(b,46+i*16,60,2);put(b,52+i*16,82*60*2);}
    for(unsigned i=0;i<82*60;++i)put(b,payload+i*2,0xc53a,2);
    const char*text="SYNTHETIC_MAP_WINDOW";
    for(unsigned i=0;i<ContentStrings;++i){put(b,stringdir+i*8,unsigned(b.size()));put(b,stringdir+i*8+4,unsigned(std::strlen(text)));b.insert(b.end(),text,text+std::strlen(text)+1);}
    for(unsigned i=0;i<AnimationKinds;++i){put(b,clipdir+i*16+4,1,2);put(b,clipdir+i*16+6,33,2);}
    for(unsigned i=0;i<CardKinds;++i)for(unsigned j=0;j<4;++j)put(b,carddir+i*16+j*4,j?i*3+j-1:i);
    for(unsigned i=0;i<U_Count;++i)put(b,uidir+i*4,i==U_CampFlames?A_Count:i);
    put(b,12,unsigned(b.size()));uint32_t hash=2166136261u;
    for(unsigned i=40;i<b.size();++i){hash^=b[i];hash*=16777619u;}put(b,36,hash);return b;
}
static bool store(const State&s){return save(s,disk);}
static bool restore(State&s){return disk.size&&load(s,disk.bytes,disk.size);}
static void snapshot(){CHECK(validate(app.game));CHECK(save(app.game,before));}
static void unchanged(){CHECK(validate(app.game));CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static void advance_ms(unsigned elapsed){while(elapsed){const unsigned n=elapsed>173?173:elapsed;app.advance(n);elapsed-=n;}}
static unsigned nodes(){unsigned n=0;for(int i=0;i<app.hit_count;++i)n+=app.hits[i].kind==HitKind::WorldNode&&app.hits[i].enabled;return n;}
static HitRegion hit(HitKind kind,int value){
    app.render();const HitRegion*found=nullptr;
    for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==kind&&app.hits[i].value==value&&app.hits[i].enabled){CHECK(!found);found=&app.hits[i];}
    CHECK(found);return *found;
}
static void click(HitKind kind,int value){const auto h=hit(kind,value);app.pointer_move(h.x+h.w/2,h.y+h.h/2);app.pointer_button(true);app.pointer_button(false);}
static void capture(const char*name){
    if(!screenshots)return;app.pointer.visible=false;app.render();const std::string path=std::string(screenshots)+"/"+name+".ppm";
    FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");unsigned char row[ScreenW*3];
    for(int y=0;y<ScreenH;++y){for(int x=0;x<ScreenW;++x){const uint16_t p=app.pixels[y*ScreenW+x];row[x*3]=uint8_t((p>>11)*255/31);row[x*3+1]=uint8_t(((p>>5)&63)*255/63);row[x*3+2]=uint8_t((p&31)*255/31);}CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static void present(){
    app.title=app.paused=app.quit=app.help=app.library=app.character_select=false;
    app.settings_open=app.map_preview.open=false;app.frontend_confirm=app.frontend_choice=0;
    app.map_clock_ms=1000;app.animation=app.tick_remainder=0;app.fast_mode=false;
    app.reset_presentation();app.pointer.visible=app.pointer.dispatching=false;app.forget_run_checkpoint();
}
static bool leads_to(const reference::ReferenceMap&m,unsigned point,reference::PointType type,unsigned depth=0){
    if(point>=m.count||depth>reference::MaxRows+2)return false;
    if(m.points[point].type==type)return true;
    const auto&children=m.points[point].children;
    for(unsigned i=0;i<children.used;++i)if(leads_to(m,children.slots[i],type,depth+1))return true;
    return false;
}
static void finish_fixture(){
    // Explicit completed outcomes are supplied only to route to the room
    // under test. Its entry, interactions, saves and exit use real handlers.
    if(app.game.original_events.active){app.game.original_events=OriginalEventState{};app.game.phase=Phase::Event;}
    if(world_room_is_combat(app.game))app.game.phase=Phase::Reward;
    if(app.game.phase==Phase::Treasure)app.game.treasure_state=2;
    CHECK(world_finish_room(app.game));CHECK(validate(app.game));
}
static void begin_room(WorldRoom wanted){
    new_run(app.game,0x7195c3,false,Character::Ironclad);CHECK(world_set_seed(app.game,"NSPIRE-WORLD-017"));CHECK(world_begin_run(app.game));
    const auto type=wanted==WorldRoom::Shop?reference::PointType::Shop:wanted==WorldRoom::Rest?reference::PointType::Rest:
        wanted==WorldRoom::Treasure?reference::PointType::Treasure:reference::PointType::Monster;
    for(unsigned guard=0;guard<100;++guard){
        if(app.game.world.room_type==unsigned(wanted)&&app.game.world.room_open){present();CHECK(validate(app.game));return;}
        if(!world_can_travel(app.game))finish_fixture();
        uint8_t ids[reference::Columns];const unsigned count=world_available_points(app.game,ids,reference::Columns);CHECK(count);
        const auto&m=*world_current_map(app.game);unsigned pick=ids[0];
        for(unsigned i=0;i<count;++i)if(leads_to(m,ids[i],type)){pick=ids[i];break;}
        CHECK(world_choose_point(app.game,pick));
    }
    CHECK(false);
}
static void close_window(){
    CHECK(app.map_window.is_open&&app.panel==3);app.input(Key::Back);
    CHECK(!app.map_window.is_open&&app.map_window.visible&&!app.panel&&!app.paused);
    advance_ms(350);CHECK(!app.map_window.visible);
}
static void hud_window(bool fast){
    begin_room(WorldRoom::Monster);app.fast_mode=fast;CHECK(!world_can_travel(app.game));
    snapshot();app.render();const std::vector<uint16_t> room(app.pixels,app.pixels+ScreenW*ScreenH);
    const auto phase=app.game.phase;const unsigned point=app.game.world.current_point;
    app.input(Key::Map);CHECK(app.panel==3&&app.map_window.is_open&&!app.map_routes_active());
    const uint32_t opened=app.map_window.start;app.render();CHECK(!nodes());
    // At time zero all authored map modulation is transparent. Inspect an
    // interior region excluding independent TopBar/Back control chrome.
    for(unsigned y=50;y<200;++y)for(unsigned x=8;x<280;++x)CHECK(app.pixels[y*320+x]==room[y*320+x]);
    unchanged();advance_ms(125);CHECK(app.map_clock_ms-opened==125);
    CHECK(app.map_window.sample(app.map_clock_ms).map.alpha==2048);app.render();capture(fast?"map-open-fast-125":"map-open-normal-125");
    app.input(Key::Confirm);CHECK(!app.map_travel.pending);unchanged();
    app.input(Key::Inspect);CHECK(app.panel==1&&app.map_window.is_open);advance_ms(100);
    app.input(Key::Map);CHECK(app.panel==3&&app.map_window.is_open&&app.map_window.start==opened);
    app.input(Key::Collection);CHECK(app.library&&app.map_window.is_open);
    app.input(Key::Map);CHECK(!app.library&&app.map_window.is_open&&app.map_window.start==opened);
    app.input(Key::Back);CHECK(!app.map_window.is_open&&app.map_window.visible&&!app.panel);
    const uint32_t closed=app.map_window.start;app.input(Key::Map);CHECK(!app.map_window.is_open&&app.map_window.start==closed);
    app.render();bool hand=false;for(int i=0;i<app.hit_count;++i)hand|=app.hits[i].kind==HitKind::Hand;CHECK(hand);
    advance_ms(349);CHECK(app.map_window.visible);app.advance(1);CHECK(!app.map_window.visible);
    CHECK(app.game.phase==phase&&app.game.world.current_point==point);unchanged();
    // A held room press is invalidated by opening the independent window.
    app.render();const auto h=hit(HitKind::Command,int(Key::Map));app.pointer_move(h.x+h.w/2,h.y+h.h/2);app.pointer_button(true);
    app.input(Key::Map);CHECK(app.map_window.is_open&&!app.pointer.held);app.pointer_button(false);CHECK(app.map_window.is_open);unchanged();
    CHECK(app.save_current_run());app.input(Key::Load);CHECK(!app.map_window.is_open&&!app.map_window.visible&&!app.panel);unchanged();
}
static void retained_shop(){
    begin_room(WorldRoom::Shop);app.game.gold=9999;CHECK(world_can_travel(app.game));snapshot();
    app.input(Key::Map);CHECK(app.map_routes_active());app.render();CHECK(nodes());unchanged();close_window();
    app.ui_action(UiAction::OpenShop);CHECK(app.shop_open);app.cursor=0;
    const unsigned deck=app.game.deck_count;app.input(Key::Confirm);CHECK(app.game.deck_count==deck+1&&(app.game.shop_sold&1));
    app.input(Key::Back);CHECK(!app.shop_open);const unsigned completed=app.game.world.rooms_completed;
    app.ui_action(UiAction::LeaveShop);CHECK(app.game.phase==Phase::Shop&&app.game.world.room_open==2&&app.game.world.rooms_completed==completed+1&&app.map_window.is_open);
    snapshot();close_window();unchanged();app.ui_action(UiAction::OpenShop);CHECK(app.shop_open);
    app.cursor=1;app.input(Key::Confirm);CHECK(app.game.deck_count==deck+2&&(app.game.shop_sold&2)&&!app.map_window.is_open);
    app.input(Key::Back);snapshot();app.ui_action(UiAction::LeaveShop);CHECK(app.map_window.is_open&&app.game.world.rooms_completed==completed+1);unchanged();
    const int chosen=app.cursor;app.render();CHECK(app.cursor==chosen);capture("map-retained-merchant");
    // Reload a saved retained shop into the same identity: load cancels UI
    // effects and keeps purchases; it must not be mistaken for a new Proceed.
    CHECK(app.save_current_run());app.input(Key::Load);CHECK(!app.map_window.visible&&app.game.phase==Phase::Shop&&app.game.world.room_open==2);unchanged();
}
static void retained_rest_and_travel(){
    begin_room(WorldRoom::Rest);CHECK(gain_world_relic(app.game,WorldRelicMiniatureTent));app.game.hp=20;
    app.cursor=0;app.input(Key::Confirm);CHECK(app.game.phase==Phase::Camp&&world_rest_can_proceed(app.game)&&!app.map_window.visible);
    CHECK(app.camp_options(nullptr,0)==1);const int healed=app.game.hp;const unsigned completed=app.game.world.rooms_completed;
    app.ui_action(UiAction::LeaveCamp);CHECK(app.game.phase==Phase::Camp&&app.game.world.room_open==2&&app.map_routes_active());
    CHECK(app.game.world.rooms_completed==completed+1);snapshot();close_window();unchanged();
    // Tent's unused Smith remains interactive after closing the map. The
    // ordinary Upgrade cancellation restores that same retained rest room.
    app.cursor=1;app.input(Key::Confirm);CHECK(app.game.phase==Phase::Upgrade&&!app.map_window.visible);app.input(Key::Back);
    CHECK(app.game.phase==Phase::Camp&&app.game.hp==healed&&app.camp_options(nullptr,0)==1);
    app.ui_action(UiAction::LeaveCamp);CHECK(app.map_routes_active()&&app.game.world.rooms_completed==completed+1);
    uint8_t ids[reference::Columns];const unsigned n=world_available_points(app.game,ids,reference::Columns);CHECK(n);
    const auto&m=*world_current_map(app.game);const unsigned destination=ids[n-1];app.cursor=m.points[destination].col;app.render();
    CHECK(app.cursor==m.points[destination].col&&nodes()==n);snapshot();
    oracle=app.game;CHECK(world_choose_point(oracle,destination));SaveData expected{};CHECK(save(oracle,expected));
    click(HitKind::WorldNode,int(destination));CHECK(app.map_travel.pending&&app.game.phase==Phase::Camp&&app.map_window.is_open);unchanged();
    advance_ms(1099);unchanged();app.advance(1);CHECK(!app.map_travel.pending&&!app.map_window.visible&&app.game.world.current_point==destination);
    CHECK(save(app.game,after));CHECK(after.size==expected.size&&!std::memcmp(after.bytes,expected.bytes,after.size));
    CHECK(app.game.world.rooms_completed==completed+1); // no second exit count
}
static void retained_claimed_treasure(){
    begin_room(WorldRoom::Treasure);app.game.world.treasure_relic=WorldRelicVajra;
    const unsigned count=app.game.world_relics.count;CHECK(!world_can_travel(app.game));snapshot();
    app.ui_action(UiAction::TreasureLeave);unchanged();CHECK(!app.map_window.visible);
    app.input(Key::Map);app.render();CHECK(!app.map_routes_active()&&!nodes());unchanged();close_window();
    click(HitKind::UiAction,int(UiAction::TreasureOpen));
    CHECK(app.game.treasure_state==1&&app.game.world_relics.count==count&&!world_can_travel(app.game));
    app.cursor=0;app.input(Key::Confirm);CHECK(app.game.world_relics.count==count+1&&!app.map_window.is_open&&world_can_travel(app.game));
    CHECK(app.game.treasure_state==2);const uint32_t opening=app.treasure_open_tick,closing=app.treasure_close_tick;
    app.ui_action(UiAction::TreasureLeave);CHECK(app.game.phase==Phase::Treasure&&app.game.world.room_open==2&&app.map_window.is_open);
    snapshot();close_window();unchanged();CHECK(app.treasure_open_tick==opening&&app.treasure_close_tick==closing);
    app.ui_action(UiAction::TreasureTake);unchanged();CHECK(app.game.world_relics.count==count+1);
}
static void reset_and_wrap(){
    begin_room(WorldRoom::Monster);app.map_clock_ms=0xffffff80u;snapshot();app.input(Key::Map);advance_ms(450);
    CHECK(app.map_window.is_open&&app.map_window.sample(app.map_clock_ms).map.alpha==4096);close_window();unchanged();
    app.input(Key::Map);CHECK(app.map_window.is_open);new_run(app.game,17,true,Character::Silent);app.render();CHECK(!app.map_window.visible&&!app.panel);
    begin_room(WorldRoom::Monster);app.input(Key::Map);app.return_to_title();CHECK(app.title&&!app.map_window.visible);
}
int main(int argc,char**argv){
    actual_profile=argc>1;std::vector<uint8_t>bytes;
    if(actual_profile){FILE*f=std::fopen(argv[1],"rb");CHECK(f);bytes.resize(spire_test::pack_size(f));CHECK(std::fread(bytes.data(),1,bytes.size(),f)==bytes.size());CHECK(!std::fclose(f));}
    else bytes=synthetic_profile();
    if(argc>2)screenshots=argv[2];CHECK(app.init(bytes.data(),bytes.size(),{store,restore}));
    hud_window(false);hud_window(true);retained_shop();retained_rest_and_travel();retained_claimed_treasure();reset_and_wrap();
    std::printf("world map window UI: %u checks passed; %s content; live room/route/save lifecycle\n",checks,actual_profile?"actual":"synthetic");
}
