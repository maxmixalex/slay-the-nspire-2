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
static uint8_t circle_alpha(unsigned frame,unsigned x,unsigned y){
    // Keep a transparent logical canvas, holes and partial-alpha ink in every
    // distinct frame. A cropped atlas region is not a valid substitute.
    if(x<9||x>=39||y<7||y>=41||(x+2*y+frame*5)%13<3)return 0;
    return x<9+6*(frame+1)?255:96;
}
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
    const char*tiles[]={"map.SELECTION_RETICLE","map.CIRCLE_0","map.CIRCLE_1","map.CIRCLE_2","map.CIRCLE_3","map.CIRCLE"};
    for(unsigned tile=0;tile<6;++tile){
        unsigned image=images;for(const auto&row:image_keys)if(!std::strcmp(row.key,tiles[tile]))image=row.index;CHECK(image<images);
        const unsigned pixels=48*48,offset=unsigned(b.size());b.resize(offset+pixels*3);
        put(b,40+image*16,offset);put(b,44+image*16,48,2);put(b,46+image*16,48,2);put(b,48+image*16,1);put(b,52+image*16,pixels*3);
        for(unsigned y=0;y<48;++y)for(unsigned x=0;x<48;++x){const unsigned i=y*48+x;
            put(b,offset+i*2,tile?0xffff:reticle_color(x,y),2);
            b[offset+pixels*2+i]=tile?circle_alpha(tile-1,x,y):reticle_alpha(x,y);
        }
    }
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
    app.title=app.paused=app.library=false;app.fast_mode=app.map_directional=false;
    app.map_clock_ms=app.tick_remainder=0;app.reset_presentation();
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
    app.map_clock_ms=0xffffff9du;app.reset_map_reticles();app.render();const unsigned wrapped=app.map_reticle_current;
    CHECK(wrapped<reference::MaxPoints&&app.map_reticles[wrapped].start==0xffffff9du);ticks(3);
    CHECK(app.map_clock_ms==0&&app.map_reticle_scale(wrapped)==3992);mouse_mode();ticks(7);
    CHECK(!app.map_reticle_opacity(wrapped));app.map_clock_ms+=0x70000000u;app.render();CHECK(!app.map_reticle_opacity(wrapped));unchanged();
    app.input(Key::Right);CHECK(app.map_directional);CHECK(load(app.game,before.bytes,before.size));app.reset_presentation();
    CHECK(app.map_directional&&app.map_reticle_act==255&&app.map_reticle_current==reference::NoPoint);
    for(const auto&r:app.map_reticles)CHECK(!r.mode);
    app.render();CHECK(app.map_reticle_seed==app.game.world.run_seed&&app.map_reticle_act==app.game.world.act_index);unchanged();
    // A stale presentation identity can share all low 32 seed bits. Leave the
    // valid run untouched and make only the remembered App identity differ.
    const unsigned stale=(app.map_reticle_current+1)%reference::MaxPoints;
    app.map_reticles[stale]={app.map_clock_ms,3900,200,2};app.map_reticle_seed^=uint64_t(1)<<32;
    CHECK(uint32_t(app.map_reticle_seed)==uint32_t(app.game.world.run_seed)&&app.map_reticle_seed!=app.game.world.run_seed);
    app.sync_map_reticles();CHECK(app.map_reticle_seed==app.game.world.run_seed&&!app.map_reticle_opacity(stale));unchanged();
    // Fast mode doubles the older animation tick, but not the original
    // selection/deselection tweens on the real map millisecond clock.
    unsigned selected_scale[2]{},dismissed_scale[2]{},dismissed_alpha[2]{};
    for(unsigned fast=0;fast<2;++fast){
        app.fast_mode=fast;app.map_clock_ms=10000;app.tick_remainder=0;app.map_directional=true;
        app.reset_map_reticles();app.render();const unsigned point=app.map_reticle_current;
        CHECK(point<reference::MaxPoints&&app.map_reticles[point].start==10000);
        const uint32_t animation=app.animation;app.advance(99);app.render();
        CHECK(app.map_clock_ms==10099&&uint32_t(app.animation-animation)==(fast?6u:3u));
        selected_scale[fast]=app.map_reticle_scale(point);mouse_mode();app.advance(99);app.render();
        dismissed_scale[fast]=app.map_reticle_scale(point);dismissed_alpha[fast]=app.map_reticle_opacity(point);
        CHECK(app.map_clock_ms==10198&&dismissed_alpha[fast]>0&&dismissed_alpha[fast]<255);
    }
    CHECK(selected_scale[0]==3992&&selected_scale[0]==selected_scale[1]);
    CHECK(dismissed_scale[0]==dismissed_scale[1]&&dismissed_alpha[0]==dismissed_alpha[1]);
    app.fast_mode=false;unchanged();
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
    // Set the test clock before reset_presentation binds combat_linger_until.
    // Rewinding only animation afterwards would draw a stale combat screen.
    app.animation=4000;start();finish_routing_fixture();app.fast_mode=false;app.pointer.visible=false;app.map_directional=false;app.reset_map_reticles();
    app.render();std::vector<uint16_t>expected(app.pixels,app.pixels+320*240);
    WorldMapLayout layout;CHECK(world_map_layout(app.game,layout));const auto view=world_map_view(app.game,layout,app.map_scroll);
    app.map_directional=true;app.sync_map_reticles();const unsigned selected=app.map_reticle_current;CHECK(selected<reference::MaxPoints);
    CHECK(world_current_map(app.game)->points[selected].type!=reference::PointType::Boss&&world_current_map(app.game)->points[selected].type!=reference::PointType::Start);
    reticle_pixel_oracle(expected,layout.points[selected].x,layout.points[selected].y+view.origin,84,84,.22,255);
    app.render();for(unsigned i=0;i<expected.size();++i)CHECK(app.pixels[i]==expected[i]);
    // Full-frame comparison covers texture colors, four mirror directions,
    // transparent/half-alpha texels, shadow order and the map clipping region.
}
static double map_expo(unsigned elapsed,unsigned duration){return elapsed>=duration?1:1.001*(1-std::pow(2.0,-10.0*elapsed/duration));}
static void map_visual_timing(){
    start();finish_routing_fixture();snapshot();const auto&m=*world_current_map(app.game);
    uint8_t ids[reference::Columns];CHECK(world_available_points(app.game,ids,reference::Columns));
    const unsigned from=app.game.world.current_point,to=ids[0];CHECK(from==m.root&&world_map_path_dot_count(app.game,from,to)>0);
    MapPresentationState motion;motion.bind(app.game,0);motion.set_open(app.game,true,0);
    CHECK(motion.seed==app.game.world.run_seed&&motion.act==app.game.world.act_index);
    CHECK(!motion.circle(from,0).alpha&&motion.marker(0).point==reference::NoPoint);
    // Neow's initial marker is an explicit new-run hook, not an inference
    // from a restored starting node. Open itself excludes that root row.
    motion.initial_marker(from,100);
    for(unsigned t=0;t<=950;++t){const auto marker=motion.marker(100+t);CHECK(marker.point==from&&marker.scale_y==4096);
        const int scale=t>=200?4096:int(std::lround(t*4096.0/200));
        double source_y=0;
        if(t<200)source_y=-25*(1-std::cos(t/200.0*std::acos(-1.0)/2));
        else if(t<950){const double phase=(t-200)/750.0;source_y=25*std::pow(2.0,-10*phase)*std::sin((phase-.075)*2*std::acos(-1.0)/.3);}
        CHECK(std::abs(int(marker.scale_x)-scale)<=1);
        // Linear interpolation of the 256-interval ELASTIC table stays
        // within 96 Q12 units, less than .024 original source pixels.
        CHECK(std::abs(marker.source_y-int(std::lround(source_y*4096)))<=96);
    }
    CHECK(motion.marker(400).source_y>0); // original ELASTIC overshoot is retained
    motion.update(app.game,1050);CHECK(motion.marker_mode==1&&motion.marker(1050).scale_x==4096);
    const uint32_t began=2000;motion.begin(app.game,from,to,began,800);
    CHECK(motion.traveling()&&motion.target()==to&&motion.traveled(app.game,to));
    CHECK(!app.game.world.visited[0][to]&&motion.points[to].circle_start==began);
    motion.begin(app.game,from,to,began+10,300);CHECK(motion.travel_start==began&&motion.travel_path_ms==800);
    for(unsigned t=0;t<=1000;++t){const auto circle=motion.circle(to,began+t);
        const unsigned frame=t<167?t*24/1000:4;
        CHECK(circle.frame==frame&&std::abs(int(circle.alpha)-int(std::lround(242.25*map_expo(t,500))))<=1);
        CHECK(std::abs(int(circle.scale)-int(std::lround((2-map_expo(t,1000))*4096)))<=1);
    }
    for(unsigned i=0;i<7;++i){const unsigned deadline=((i+1)*800+6)/7;
        const auto before_dot=motion.path_dot(app.game,from,to,i,7,began+deadline-1);
        const auto first_dot=motion.path_dot(app.game,from,to,i,7,began+deadline);
        CHECK(!before_dot.traveled&&!before_dot.fresh&&before_dot.scale==4096);
        CHECK(first_dot.traveled&&first_dot.fresh&&first_dot.scale==6963);
        for(unsigned t=0;t<=400;++t){const auto dot=motion.path_dot(app.game,from,to,i,7,began+deadline+t);
            const double remaining=1-t/400.0;
            CHECK(dot.fresh&&dot.traveled&&std::abs(int(dot.scale)-int(std::lround((1.2+.5*remaining*remaining*remaining)*4096)))<=1);
        }
    }
    motion.update(app.game,began+100);CHECK(motion.marker(began+100).point==from&&motion.marker(began+100).scale_x==2048);
    motion.update(app.game,began+200);CHECK(motion.marker(began+200).point==reference::NoPoint);
    motion.update(app.game,began+799);CHECK(motion.marker(began+799).point==reference::NoPoint);
    motion.update(app.game,began+800);CHECK(motion.marker_start==began+800&&motion.marker(began+800).point==to&&motion.marker(began+800).scale_x==0);
    unchanged();
    // Core entry is deliberately explicit in this visual-only fixture. It
    // does not erase the last dot's 400ms tail or the target marker entry.
    CHECK(world_choose_point(app.game,to));CHECK(validate(app.game));motion.committed(app.game,began+1100);
    CHECK(!motion.traveling()&&motion.circle(to,began+1100).frame==4&&motion.circle(to,began+1100).scale==4096);
    CHECK(motion.path_dot(app.game,from,to,6,7,began+1100).scale>4915);
    CHECK(motion.marker_start==began+800&&motion.marker(began+1100).point==to);
    motion.update(app.game,began+1200);CHECK(!motion.points[to].path_ms&&motion.path_dot(app.game,from,to,6,7,began+1200).fresh);
    motion.update(app.game,began+1750);CHECK(motion.marker_mode==1&&motion.marker(began+1750).source_y==0);snapshot();
    CHECK(load(app.game,before.bytes,before.size));motion.reset();motion.bind(app.game,10000);
    const auto restored=motion.circle(to,10000);CHECK(restored.frame==4&&restored.alpha==242&&restored.scale==4096);
    const auto old_path=motion.path_dot(app.game,from,to,0,7,10000);CHECK(old_path.traveled&&!old_path.fresh&&old_path.scale==4915);
    CHECK(motion.marker(10000).point==reference::NoPoint);
    motion.set_open(app.game,true,10000);CHECK(motion.marker(10000).point==to&&motion.marker(10000).scale_x==0);
    motion.set_open(app.game,true,10050);CHECK(motion.marker_start==10000&&motion.marker_mode==3);
    motion.set_open(app.game,false,10100);motion.set_open(app.game,true,10100);
    CHECK(motion.marker_start==10000&&motion.marker_mode==1&&motion.marker(10100).scale_x==4096);unchanged();

    // A second quick route owns a second circle; it cannot replace the
    // still-running first circle or restart that first node on redraw.
    start();finish_routing_fixture();CHECK(world_available_points(app.game,ids,reference::Columns));
    const unsigned first=ids[0],root=app.game.world.current_point;motion.reset();motion.begin(app.game,root,first,0,300);
    CHECK(world_choose_point(app.game,first));motion.committed(app.game,600);finish_routing_fixture();
    CHECK(motion.points[first].circle_mode==1&&world_available_points(app.game,ids,reference::Columns));
    const unsigned second=ids[0];motion.set_open(app.game,true,600);snapshot();motion.begin(app.game,first,second,601,300);
    CHECK(motion.points[first].circle_start==0&&motion.points[second].circle_start==601);
    CHECK(motion.circle(first,701).alpha&&motion.circle(second,701).alpha&&motion.circle(first,701).scale>4096);
    motion.update(app.game,1601);CHECK(motion.points[first].circle_mode==2&&motion.points[second].circle_mode==2);
    motion.cancel(app.game,1700);CHECK(!motion.pending&&!motion.circle(second,1700).alpha&&motion.circle(first,1700).alpha==242);
    CHECK(motion.marker(1700).point==first&&motion.marker(1700).scale_x==0);unchanged();
    // Original Open has no IsTraveling guard. A real map close/reopen during
    // Hide fast-forwards it and reveals current again. Target SetMapPoint
    // later fast-forwards that entry but cannot move an already-visible
    // marker. Ordinary covering panels must not manufacture this open edge.
    motion.begin(app.game,first,second,1800,300);motion.set_open(app.game,false,1850);motion.set_open(app.game,true,1900);
    CHECK(motion.pending&&motion.points[second].circle_start==1800&&motion.marker_start==1900);
    CHECK(motion.marker(1900).point==first&&motion.marker(1900).scale_x==0);
    motion.update(app.game,2100);CHECK(!motion.marker_waiting&&motion.marker_mode==1&&motion.marker(2100).point==first);
    CHECK(motion.marker_start==1900&&motion.points[second].path_start==1800);unchanged();
    // All three effects use bounded unsigned real-ms differences across
    // wrap. Completed modes latch instead of restarting on a later wrap.
    const uint32_t wrap=0xffffff00u;motion.reset();motion.bind(app.game,wrap);motion.initial_marker(first,wrap-950);
    motion.begin(app.game,first,second,wrap,300);motion.update(app.game,wrap+300);
    CHECK(motion.marker_start==uint32_t(wrap+300)&&motion.marker(wrap+300).point==second);
    motion.update(app.game,wrap+1250);CHECK(motion.points[second].circle_mode==2&&motion.marker_mode==1);
    CHECK(motion.circle(second,wrap+1250).scale==4096&&motion.path_dot(app.game,first,second,0,3,wrap+1250).scale==4915);
    motion.update(app.game,uint32_t(wrap+1250+0x70000000u));CHECK(motion.points[second].circle_mode==2&&motion.marker_mode==1);
    motion.seed^=uint64_t(1)<<32;CHECK(uint32_t(motion.seed)==uint32_t(app.game.world.run_seed));
    motion.update(app.game,20000);CHECK(motion.seed==app.game.world.run_seed&&!motion.pending&&!motion.circle(second,20000).alpha);
    CHECK(motion.circle(first,20000).frame==4&&motion.marker(20000).point==reference::NoPoint);unchanged();
    // Free travel without a graph edge has no fabricated path delay. Set
    // immediately fast-forwards the old hide, then starts the new marker.
    motion.initial_marker(first,21000-950);motion.begin(app.game,reference::NoPoint,second,21000,800);
    CHECK(motion.pending&&!motion.travel_path_ms&&!motion.marker_waiting);
    CHECK(motion.marker(21000).point==second&&motion.marker(21000).scale_x==0);
    CHECK(!motion.path_dot(app.game,first,second,0,3,21100).traveled);unchanged();
}
static double circle_oracle_sine(int degrees){
    int phase=((degrees%360+360)%360)*256/360,sign=phase>=128?-1:1;phase%=128;if(phase>64)phase=128-phase;
    const unsigned i=unsigned(phase)/4,j=unsigned(phase)%4;
    const int left=int(std::lround(std::sin(i*std::acos(-1.0)/32)*1024));
    const int right=i==16?1024:int(std::lround(std::sin((i+1)*std::acos(-1.0)/32)*1024));
    return sign*((left*int(4-j)+right*int(j))/4)/1024.0;
}
static void circle_pixels(){
    if(actual_profile)return;
    app.animation=4000;start();finish_routing_fixture();app.pointer.visible=false;app.render();snapshot();
    const auto&m=*world_current_map(app.game);uint8_t ids[reference::Columns];CHECK(world_available_points(app.game,ids,reference::Columns));
    const unsigned point=ids[0];WorldMapLayout layout;CHECK(world_map_layout(app.game,layout));
    app.map_visuals.begin(app.game,app.game.world.current_point,point,1000,800);
    reference::Random circle;circle.reset(app.game.world.run_seed+uint64_t(m.points[point].row)*132u);
    const int rotation=world_float_truncate(world_random_float(circle,0,360));
    const auto final_size=world_float_round(reference::math::Real::integer(44)*world_map_random_float(circle,world_float(0x3f59999aU),world_float(0x3f666666U)));
    const double sine=circle_oracle_sine(rotation),cosine=circle_oracle_sine(rotation+90);
    const uint16_t ink=uint16_t((36u>>3)<<11|(31u>>2)<<5|(26u>>3));
    const unsigned ages[]={21,42,84,125,167};
    for(unsigned frame=0;frame<5;++frame)for(int scroll=0;scroll<2;++scroll){
        app.map_clock_ms=1000+ages[frame];app.map_scroll=scroll;const auto saved=app.map_visuals.points[point];
        app.map_visuals.points[point].circle_start=app.map_clock_ms;app.render();
        const std::vector<uint16_t>base(app.pixels,app.pixels+320*240);
        app.map_visuals.points[point]=saved;app.render();
        const auto sample=app.map_visuals.circle(point,app.map_clock_ms);CHECK(sample.frame==frame&&sample.alpha);
        const int size=world_map_round(world_float_round(final_size*reference::math::Real::integer(sample.scale)/reference::math::Real::integer(4096)));
        const auto view=world_map_view(app.game,layout,app.map_scroll);const int cx=layout.points[point].x,cy=layout.points[point].y+view.origin;
        unsigned changed=0,exact=0;
        for(int y=0;y<240;++y)for(int x=0;x<320;++x){const unsigned at=unsigned(y*320+x);unsigned opacity=0;
            const double dx=x+.5-cx,dy=y+.5-cy,u=dx*cosine+dy*sine+size*.5,v=dy*cosine-dx*sine+size*.5;
            if(y>=WorldMapTop&&y<WorldMapBottom&&u>=0&&v>=0&&u<size&&v<size){
                const unsigned tx=unsigned(u*48/size),ty=unsigned(v*48/size);
                opacity=unsigned(std::lround(circle_alpha(frame,tx,ty)*sample.alpha/255.0));
            }
            if(app.pixels[at]!=base[at]){++changed;CHECK(opacity>0);if(app.pixels[at]==reticle_mix(base[at],ink,opacity))++exact;}
        }
        // Other map children can occlude a circle drawn earlier in scene
        // order. Every changed pixel must still lie in this frame's rotated
        // alpha footprint, and visible unoccluded texels must blend exactly.
        CHECK(changed>10&&exact>10);unchanged();
    }
    capture("map-circle-entry");
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
    // A real click keeps the route visible until the original travel fade
    // completes, then enters exactly that node. No State changes on press.
    h=find_hit(HitKind::WorldNode,ids[0]);CHECK(h);app.pointer_move(h->x+h->w/2,h->y+h->h/2);app.pointer_button(true);app.pointer_button(false);
    CHECK(app.map_travel.pending&&app.game.phase==Phase::Map&&app.game.world.current_point==m.root);unchanged();
    for(unsigned i=0;i<10;++i)app.advance(250);CHECK(!app.map_travel.pending&&!app.map_travel.fading_in);
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
    if(argc>2)screenshots=argv[2];CHECK(app.init(pack.data(),pack.size(),{}));reticle_timing();reticle_pixels();map_visual_timing();circle_pixels();layout_and_input();boss_cycles_and_regions();
    std::printf("world map UI: %u checks passed; %s profile\n",checks,argc>1?"actual":"synthetic");
}
