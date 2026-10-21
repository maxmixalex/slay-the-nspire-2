#include "app.hpp"
#include "original_events.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
using namespace spire;
static App app;
static SaveData before,after;
static unsigned checks;
static const char* screenshots;
static bool actual_profile;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"map UI: %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static const struct ImageKey {const char*key;unsigned index;} image_keys[]={
#include "../src/world_image_index.inc"
};
static uint16_t reticle_color(unsigned x,unsigned y){const uint16_t colors[]={0xf800,0x07e0,0x001f,0xffff};return colors[(x>=24)+2*(y>=24)];}
static uint8_t reticle_alpha(unsigned x,unsigned y){return x>=12&&x<36&&y>=12&&y<36?0:x<24?255:128;}
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
    unsigned reticle=images;for(const auto&row:image_keys)if(!std::strcmp(row.key,"map.SELECTION_RETICLE"))reticle=row.index;CHECK(reticle<images);
    const unsigned pixels=48*48,offset=unsigned(b.size());b.resize(offset+pixels*3);
    put(b,40+reticle*16,offset);put(b,44+reticle*16,48,2);put(b,46+reticle*16,48,2);put(b,48+reticle*16,1);put(b,52+reticle*16,pixels*3);
    for(unsigned y=0;y<48;++y)for(unsigned x=0;x<48;++x){const unsigned i=y*48+x;put(b,offset+i*2,reticle_color(x,y),2);b[offset+pixels*2+i]=reticle_alpha(x,y);}
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
static void ticks(unsigned count){while(count--)app.advance(33);}
static void mouse_mode(){app.pointer_move(app.pointer.x?0:1,app.pointer.y?0:1);app.pointer.visible=false;}
static void reticle_timing(){
    start();finish_routing_fixture();app.fast_mode=false;app.map_directional=false;app.render();snapshot();
    CHECK(app.map_reticle_current==reference::NoPoint);
    for(unsigned i=0;i<reference::MaxPoints;++i)CHECK(!app.map_reticle_opacity(i));
    // Ordinary keyboard commands do not acquire directional-navigation focus.
    app.input(Key::Detail);CHECK(app.map_legend&&!app.map_directional);app.input(Key::Back);CHECK(!app.map_legend);
    app.input(Key::Right);app.render();const unsigned first=app.map_reticle_current;
    CHECK(app.map_directional&&first<world_current_map(app.game)->count&&world_reachable(app.game,first));
    CHECK(app.map_reticle_opacity(first)==255&&app.map_reticle_scale(first)==4096);
    const uint32_t began=app.map_reticles[first].start;
    for(unsigned t=1;t<=16;++t){ticks(1);app.render();
        const double elapsed=t*33.0;
        const double progress=elapsed>=500?1:1.001*(1-std::pow(2.0,-10*elapsed/500));
        const int expected=int(std::lround((.9+.1*progress)*4096));
        CHECK(std::abs(int(app.map_reticle_scale(first))-expected)<=1&&app.map_reticle_opacity(first)==255);
        CHECK(app.map_reticles[first].start==began);
    }
    for(unsigned i=0;i<5;++i)app.render();CHECK(app.map_reticles[first].start==began);unchanged();
    capture("map-original-selected-reticle");
    // Mouse motion relinquishes focus. Keep all original alpha holes while
    // fading from the actual current scale, rather than dropping the old box.
    mouse_mode();CHECK(!app.map_directional&&app.map_reticle_current==reference::NoPoint);
    CHECK(app.map_reticle_opacity(first)==255&&app.map_reticle_scale(first)==4096);
    const uint32_t dismissed=app.map_reticles[first].start;
    for(unsigned t=1;t<=7;++t){ticks(1);app.render();
        const double progress=t*33>=200?1:std::sin(t*33.0/200*std::acos(-1.0)/2);
        CHECK(std::abs(int(app.map_reticle_opacity(first))-int(std::lround(255*(1-progress))))<=1);
        if(t<7)CHECK(std::abs(int(app.map_reticle_scale(first))-int(std::lround((1+.05*progress)*4096)))<=1);
        CHECK(app.map_reticles[first].start==dismissed);
    }
    CHECK(!app.map_reticle_opacity(first));unchanged();
    // A second focus switch does not replace another node's independent fade.
    uint8_t ids[reference::Columns];const unsigned count=world_available_points(app.game,ids,reference::Columns);CHECK(count);
    app.input(Key::Right);const unsigned a=app.map_reticle_current;ticks(2);const unsigned from=app.map_reticle_scale(a);
    if(count>1){
        app.input(Key::Right);const unsigned b=app.map_reticle_current;CHECK(a!=b);
        CHECK(app.map_reticle_opacity(a)==255&&app.map_reticle_scale(a)==from);
        CHECK(app.map_reticle_opacity(b)==255&&app.map_reticle_scale(b)==4096);ticks(1);
        app.input(Key::Right);const unsigned c=app.map_reticle_current;CHECK(c!=b);
        CHECK(app.map_reticle_opacity(b)==255&&app.map_reticle_opacity(c)==255);
        if(count>2)CHECK(c!=a&&app.map_reticle_opacity(a)>0&&app.map_reticle_opacity(a)<255);
        else CHECK(c==a&&app.map_reticle_scale(a)==4096); // interrupted fade is killed on reselection
    }
    // Pointer buttons also switch modes when no coordinate motion occurs.
    app.pointer_move(5,5);app.pointer.visible=false;app.input(Key::Right);CHECK(app.map_directional);
    app.pointer_button(true);CHECK(!app.map_directional&&app.map_reticle_current==reference::NoPoint);app.pointer_button(false);
    app.input(Key::Right);const unsigned active=app.map_reticle_current;CHECK(active<reference::MaxPoints);
    app.input(Key::Detail);CHECK(app.map_legend&&app.map_reticle_current==reference::NoPoint);
    app.input(Key::Back);CHECK(!app.map_legend&&app.map_reticle_opacity(app.map_reticle_current)==255);
    app.input(Key::Map);CHECK(app.panel==3&&app.map_reticle_current==reference::NoPoint);
    app.input(Key::Back);CHECK(!app.panel&&app.map_reticle_current<reference::MaxPoints);unchanged();
    // A wrapped presentation clock continues the same selected/dismissed tween.
    app.animation=0xfffffffdu;app.reset_map_reticles();app.render();const unsigned wrapped=app.map_reticle_current;
    CHECK(wrapped<reference::MaxPoints&&app.map_reticles[wrapped].start==0xfffffffdu);ticks(3);
    CHECK(app.animation==0&&app.map_reticle_scale(wrapped)==3992);mouse_mode();ticks(7);
    CHECK(!app.map_reticle_opacity(wrapped));app.animation+=0x70000000u;app.render();CHECK(!app.map_reticle_opacity(wrapped));unchanged();
    app.input(Key::Right);CHECK(app.map_directional);CHECK(load(app.game,before.bytes,before.size));app.reset_presentation();
    CHECK(app.map_directional&&app.map_reticle_act==255&&app.map_reticle_current==reference::NoPoint);
    for(const auto&r:app.map_reticles)CHECK(!r.mode);
    app.render();CHECK(app.map_reticle_seed==app.game.world.run_seed&&app.map_reticle_act==app.game.world.act_index);unchanged();
    // A stale presentation identity can share all low 32 seed bits. Leave the
    // valid run untouched and make only the remembered App identity differ.
    const unsigned stale=(app.map_reticle_current+1)%reference::MaxPoints;
    app.map_reticles[stale]={app.animation,3900,200,2};app.map_reticle_seed^=uint64_t(1)<<32;
    CHECK(uint32_t(app.map_reticle_seed)==uint32_t(app.game.world.run_seed)&&app.map_reticle_seed!=app.game.world.run_seed);
    app.sync_map_reticles();CHECK(app.map_reticle_seed==app.game.world.run_seed&&!app.map_reticle_opacity(stale));unchanged();
}
static uint16_t reticle_mix(uint16_t back,uint16_t front,unsigned alpha){
    const unsigned r=((back>>11)*(255-alpha)+(front>>11)*alpha+127)/255;
    const unsigned g=(((back>>5)&63)*(255-alpha)+((front>>5)&63)*alpha+127)/255;
    const unsigned b=((back&31)*(255-alpha)+(front&31)*alpha+127)/255;
    return uint16_t(r<<11|g<<5|b);
}
static void reticle_pixel_oracle(std::vector<uint16_t>&frame,int cx,int cy,int width,int height,double scale,unsigned opacity){
    // Independent scene-space oracle: each 30x30 corner flips about its own
    // origin, and the child shadow remains (+3,+2) in unflipped world space.
    for(unsigned corner=0;corner<4;++corner){const bool fx=corner&2,fy=corner&1;
        const double x=fx?width*.5-20:-width*.5-10,y=fy?height*.5-20:-height*.5-10;
        for(unsigned pass=0;pass<2;++pass){const bool shadow=!pass;const double xx=x+(shadow?3:0),yy=y+(shadow?2:0);
            const int left=cx+int(std::lround(xx*scale)),top=cy+int(std::lround(yy*scale));
            const int right=cx+int(std::lround((xx+30)*scale)),bottom=cy+int(std::lround((yy+30)*scale));
            for(int py=top;py<bottom;++py)for(int px=left;px<right;++px){if(px<0||px>=320||py<WorldMapTop||py>=WorldMapBottom)continue;
                unsigned tx=unsigned((px-left+.5)*48/(right-left)),ty=unsigned((py-top+.5)*48/(bottom-top));if(fx)tx=47-tx;if(fy)ty=47-ty;
                const unsigned alpha=unsigned(std::lround(reticle_alpha(tx,ty)*(opacity/255.0)*(shadow?.35:1.0)));
                frame[py*320+px]=reticle_mix(frame[py*320+px],shadow?0:reticle_color(tx,ty),alpha);
            }
        }
    }
}
static void reticle_pixels(){
    if(actual_profile)return;
    start();finish_routing_fixture();app.fast_mode=false;app.animation=4000;app.pointer.visible=false;app.map_directional=false;app.reset_map_reticles();
    app.render();std::vector<uint16_t>expected(app.pixels,app.pixels+320*240);
    WorldMapLayout layout;CHECK(world_map_layout(app.game,layout));const auto view=world_map_view(app.game,layout,app.map_scroll);
    app.map_directional=true;app.sync_map_reticles();const unsigned selected=app.map_reticle_current;CHECK(selected<reference::MaxPoints);
    CHECK(world_current_map(app.game)->points[selected].type!=reference::PointType::Boss&&world_current_map(app.game)->points[selected].type!=reference::PointType::Start);
    reticle_pixel_oracle(expected,layout.points[selected].x,layout.points[selected].y+view.origin,84,84,.22,255);
    app.render();for(unsigned i=0;i<expected.size();++i)CHECK(app.pixels[i]==expected[i]);
    // Full-frame comparison covers texture colors, four mirror directions,
    // transparent/half-alpha texels, shadow order and the map clipping region.
}
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
    actual_profile=argc>1;
    std::vector<uint8_t>pack;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long size=std::ftell(f);CHECK(size>0&&size<=MaxContentFile);CHECK(!std::fseek(f,0,SEEK_SET));pack.resize(size);CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());CHECK(!std::fclose(f));}else pack=synthetic_profile();
    if(argc>2)screenshots=argv[2];CHECK(app.init(pack.data(),pack.size(),{}));reticle_timing();reticle_pixels();layout_and_input();boss_cycles_and_regions();
    std::printf("world map UI: %u checks passed; %s profile\n",checks,argc>1?"actual":"synthetic");
}
