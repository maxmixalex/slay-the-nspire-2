#include "app.hpp"
#include "original_events.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App app;
static SaveData before,after;
static unsigned checks;
static const char* screenshots;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"map UI: %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static void put(std::vector<uint8_t>&b,unsigned at,unsigned value,unsigned count=4){for(unsigned i=0;i<count;++i)b[at+i]=uint8_t(value>>(i*8));}
static std::vector<uint8_t> synthetic_profile(){
    constexpr unsigned images=CardKinds>U_Count?CardKinds:U_Count,strings=ContentStrings;
    constexpr unsigned stringdir=40+images*16,clipdir=stringdir+strings*8;
    constexpr unsigned carddir=clipdir+AnimationKinds*16,uidir=carddir+CardKinds*16,payload=uidir+U_Count*4;
    std::vector<uint8_t>b(payload+82*60*2);put(b,0,0x3250534e);put(b,4,ContentVersion);put(b,8,0x41cef1ea);
    put(b,16,images);put(b,24,strings);put(b,28,AnimationKinds);put(b,32,CardKinds);
    for(unsigned i=0;i<images;++i){put(b,40+i*16,payload);put(b,44+i*16,82,2);put(b,46+i*16,60,2);put(b,52+i*16,82*60*2);}
    for(unsigned i=0;i<82*60;++i)put(b,payload+i*2,0xc53a,2);
    for(unsigned i=0;i<strings;++i){const char*text="Synthetic map UI";put(b,stringdir+i*8,unsigned(b.size()));put(b,stringdir+i*8+4,unsigned(std::strlen(text)));b.insert(b.end(),text,text+std::strlen(text)+1);}
    for(unsigned i=0;i<AnimationKinds;++i){put(b,clipdir+i*16+4,1,2);put(b,clipdir+i*16+6,50,2);}
    for(unsigned i=0;i<CardKinds;++i)for(unsigned j=0;j<4;++j)put(b,carddir+i*16+j*4,j?i*3+j-1:i);
    for(unsigned i=0;i<U_Count;++i)put(b,uidir+i*4,i==U_CampFlames?A_Count:i);
    put(b,12,unsigned(b.size()));uint32_t hash=2166136261u;for(unsigned i=40;i<b.size();++i){hash^=b[i];hash*=16777619u;}put(b,36,hash);return b;
}
static void snapshot(){CHECK(validate(app.game));CHECK(save(app.game,before));}
static void unchanged(){CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static void capture(const char*name){
    if(!screenshots)return;app.pointer.visible=false;app.render();const std::string path=std::string(screenshots)+"/"+name+".ppm";
    FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");unsigned char row[320*3];
    for(int y=0;y<240;++y){for(int x=0;x<320;++x){const unsigned pixel=app.pixels[y*320+x];row[x*3]=uint8_t((pixel>>11)*255/31);row[x*3+1]=uint8_t(((pixel>>5)&63)*255/63);row[x*3+2]=uint8_t((pixel&31)*255/31);}CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static void start(reference::ActKind region=reference::ActKind::Overgrowth,unsigned ascension=0){
    new_run(app.game,0x175c3,false,Character::Silent);CHECK(world_set_seed(app.game,"MAP-PRESENTATION-17"));CHECK(world_begin_run(app.game,region,ascension));
    app.title=app.paused=app.library=false;app.reset_presentation();
}
static void finish_routing_fixture(){
    // Supply an explicit room result; this suite verifies map UI and history,
    // not strategies, enemy damage or the choices of individual event scripts.
    if(app.game.original_events.active){
        if(app.game.world.event_combat_parent){
            app.game.world.room_type=app.game.world.parent_room_type;
            app.game.world.parent_room_type=app.game.world.event_combat_parent=0;
        }
        app.game.original_events=OriginalEventState{};app.game.phase=Phase::Event;
    }
    if(world_room_is_combat(app.game))app.game.phase=Phase::Reward;
    CHECK(world_finish_room(app.game));CHECK(validate(app.game));app.reset_presentation();
}
static unsigned world_hits(){unsigned count=0;for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::WorldNode)++count;return count;}
static const HitRegion* find_hit(HitKind kind,int value){for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==kind&&app.hits[i].value==value)return &app.hits[i];return nullptr;}
static void layout_and_input(){
    start();CHECK(app.game.world.visited_rooms[0][app.game.world.current_point]==unsigned(WorldRoom::Event));
    app.input(Key::Map);CHECK(app.panel==3);snapshot();app.render();CHECK(!world_hits());capture("map-neow-inspection");
    app.input(Key::Detail);CHECK(app.map_legend);app.render();CHECK(!world_hits());app.input(Key::Back);CHECK(!app.map_legend&&app.panel==3&&!app.paused);
    app.input(Key::Back);CHECK(!app.panel);unchanged();finish_routing_fixture();snapshot();
    WorldMapLayout a,b;CHECK(world_map_layout(app.game,a));CHECK(world_map_layout(app.game,b));
    const auto&m=*world_current_map(app.game);unsigned regular=0;
    for(unsigned col=0;col<reference::Columns;++col)for(unsigned row=0;row<m.config.rows;++row)if(m.grid[col][row]!=reference::NoPoint)++regular;
    CHECK(a.jitter_calls==regular*2&&b.jitter_calls==a.jitter_calls);
    for(unsigned i=0;i<m.count;++i)CHECK(a.points[i].x==b.points[i].x&&a.points[i].y==b.points[i].y&&a.points[i].angle==b.points[i].angle);
    CHECK(a.points[m.root].x!=a.points[m.boss].x); // authored ancient and Boss anchors differ
    app.render();uint8_t ids[reference::Columns];const unsigned n=world_available_points(app.game,ids,reference::Columns);CHECK(n&&world_hits()==n);
    for(unsigned i=0;i<n;++i){const HitRegion*h=find_hit(HitKind::WorldNode,ids[i]);CHECK(h&&h->enabled&&h->w>=28&&h->h>=28&&h->y>=WorldMapTop&&h->y+h->h<=WorldMapBottom);}
    capture("map-start-frontier");const int first=app.cursor;
    for(unsigned i=0;i<n;++i){app.input(Key::Right);bool reachable=false;for(unsigned j=0;j<n;++j)if(m.points[ids[j]].col==app.cursor)reachable=true;CHECK(reachable);}CHECK(app.cursor==first);
    app.input(Key::Detail);CHECK(app.map_legend);const uint32_t legend_context=app.input_context();app.render();CHECK(!world_hits());
    for(unsigned i=0;i<6;++i)CHECK(find_hit(HitKind::UiAction,int(UiAction::MapLegendUnknown)+int(i)));
    app.ui_action(UiAction::MapLegendElite);CHECK(app.map_legend_item==5);app.pointer_scroll(-2);CHECK(app.map_legend_item==3);capture("map-legend");
    app.input(Key::Confirm);CHECK(!app.map_legend&&app.game.phase==Phase::Map&&app.input_context()!=legend_context);
    // A press begun on a real map node cannot turn into a room entry after a
    // modal opens. The ordinary release path must discard the stale gesture.
    app.render();const auto*h=find_hit(HitKind::WorldNode,ids[0]);CHECK(h);const int x=h->x+h->w/2,y=h->y+h->h/2;
    app.pointer_move(x,y);app.pointer_button(true);CHECK(app.pointer.held);app.map_legend=true;app.pointer_button(false);CHECK(app.game.phase==Phase::Map&&!app.pointer.held);app.map_legend=false;
    for(int i=0;i<40;++i){app.input(Key::Up);app.render();}const auto top=world_map_view(app.game,a,app.map_scroll);CHECK(top.origin==top.maximum);CHECK(!world_hits());capture("map-boss-distant");
    app.input(Key::Confirm);CHECK(app.game.phase==Phase::Map&&app.map_scroll==0);app.render();CHECK(world_hits()==n);unchanged();
    // A real click enters exactly that node after the presentation-only checks.
    h=find_hit(HitKind::WorldNode,ids[0]);CHECK(h);app.pointer_move(h->x+h->w/2,h->y+h->h/2);app.pointer_button(true);app.pointer_button(false);
    CHECK(app.game.world.current_point==ids[0]&&app.game.world.total_floor==2&&app.game.phase==Phase::Combat);
    CHECK(app.game.world.visited_rooms[0][ids[0]]==unsigned(WorldRoom::Monster));snapshot();app.input(Key::Map);app.render();CHECK(!world_hits());capture("map-current-combat");unchanged();app.input(Key::Back);
}
static void boss_cycles_and_regions(){
    CHECK(world_map_clip_duration(EncounterId::CeremonialBeastBoss)==6000&&world_map_clip_duration(EncounterId::QueenBoss)==10300&&world_map_clip_duration(EncounterId::TheInsatiableBoss)==10667);
    CHECK(world_map_clip_frame(10666,214,10667)==213&&world_map_clip_frame(10667,214,10667)==0&&world_map_clip_frame(5333,214,10667)==106);
    for(unsigned region=0;region<4;++region){
        start(region==1?reference::ActKind::Underdocks:reference::ActKind::Overgrowth,10);finish_routing_fixture();
        const unsigned act=region<2?0:region-1;
        for(unsigned guard=0;app.game.world.act_index<act&&guard<50;++guard){uint8_t ids[reference::Columns];CHECK(world_available_points(app.game,ids,reference::Columns));CHECK(world_choose_point(app.game,ids[0]));finish_routing_fixture();}
        CHECK(app.game.world.rooms[app.game.world.act_index].region==region);
        if(app.game.world.current_point==reference::NoPoint){CHECK(world_choose_point(app.game,world_current_map(app.game)->root));finish_routing_fixture();}
        const char*names[]={"OVERGROWTH","UNDERDOCKS","HIVE","GLORY"};char key[80];std::snprintf(key,sizeof key,"BG_%s_TOP",names[region]);CHECK(app.content.world_image("map",key)>=0);
        EncounterId bosses[EncounterKinds];const unsigned count=encounter_pool(region,EncounterClass::Boss,bosses,EncounterKinds);CHECK(count>0);
        auto&q=app.game.world.rooms[app.game.world.act_index];const EncounterId first=q.bosses[0],second=q.bosses[1];
        for(unsigned i=0;i<count;++i){q.bosses[0]=bosses[i];if(q.boss_count==2)q.bosses[1]=bosses[(i+1)%count];app.map_scroll=reference::MaxRows;app.render();snapshot();
            const auto*d=encounter_definition(bosses[i]);CHECK(d);if(world_map_clip_duration(bosses[i]))CHECK(app.content.world_clip("map",d->key)>=0);
            else{std::snprintf(key,sizeof key,"BOSS_%s",d->key);CHECK(app.content.world_image("map",key)>=0);}
            char shot[100];std::snprintf(shot,sizeof shot,"map-%s",d->key);capture(shot);app.advance(99);app.render();unchanged();
        }q.bosses[0]=first;q.bosses[1]=second;
    }
}
int main(int argc,char**argv){
    std::vector<uint8_t>pack;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long size=std::ftell(f);CHECK(size>0&&size<=MaxContentFile);CHECK(!std::fseek(f,0,SEEK_SET));pack.resize(size);CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());CHECK(!std::fclose(f));}else pack=synthetic_profile();
    if(argc>2)screenshots=argv[2];CHECK(app.init(pack.data(),pack.size(),{}));layout_and_input();boss_cycles_and_regions();
    std::printf("world map UI: %u checks passed; %s profile\n",checks,argc>1?"actual":"synthetic");
}
