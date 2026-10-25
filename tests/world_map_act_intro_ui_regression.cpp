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
static unsigned checks;
static bool actual_profile,disk_exists,read_ok=true,write_ok=true;
static const char* screenshots;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"map act intro UI: %s:%d %s (act=%u phase=%u clock=%u elapsed=%u banners=%u)\n",__FILE__,__LINE__,#x,app.game.world.act_index,unsigned(app.game.phase),app.map_clock_ms,app.map_act_intro.camera_elapsed,app.map_act_intro.banner_count);std::exit(1);}}while(0)
struct TextIndex{const char*key;uint16_t index;};
static const TextIndex text_indexes[]={
#include "../src/world_text_index.inc"
};
static void put(std::vector<uint8_t>&b,unsigned at,unsigned value,unsigned bytes=4){for(unsigned i=0;i<bytes;++i)b[at+i]=uint8_t(value>>(8*i));}
static std::vector<uint8_t> synthetic_profile(){
    // Neutral text and synthetic glyphs/images expose the full production
    // renderer and its hit geometry without redistributing original assets.
    constexpr unsigned images=CardKinds>U_Count?CardKinds:U_Count,glyphs=5*95;
    constexpr unsigned glyphdir=40+images*16,stringdir=glyphdir+glyphs*16;
    constexpr unsigned clipdir=stringdir+ContentStrings*8,carddir=clipdir+AnimationKinds*16;
    constexpr unsigned uidir=carddir+CardKinds*16,payload=uidir+U_Count*4;
    std::vector<uint8_t>b(payload+82*60*2);
    put(b,0,0x3250534e);put(b,4,ContentVersion);put(b,8,0x41cef1ea);put(b,16,images);put(b,20,glyphs);
    put(b,24,ContentStrings);put(b,28,AnimationKinds);put(b,32,CardKinds);
    for(unsigned i=0;i<images;++i){put(b,40+i*16,payload);put(b,44+i*16,82,2);put(b,46+i*16,60,2);put(b,52+i*16,82*60*2);}
    for(unsigned i=0;i<82*60;++i)put(b,payload+i*2,0xc53a,2);
    unsigned glyph=0;for(unsigned size:{8u,10u,12u,16u,20u})for(unsigned cp=32;cp<127;++cp,++glyph){
        const unsigned at=glyphdir+glyph*16,offset=unsigned(b.size());put(b,at,cp);put(b,at+4,offset);put(b,at+8,size,2);
        b[at+10]=5;b[at+11]=7;put(b,at+12,6,2);b.resize(offset+35);
        for(unsigned y=0;y<7;++y)for(unsigned x=0;x<5;++x)b[offset+y*5+x]=cp==' '?0:uint8_t((cp>>((x+y*3)%7)&1)?255:0);
    }
    std::vector<std::string>texts(ContentStrings,"SYNTHETIC");
    for(const auto&row:text_indexes){CHECK(row.index<texts.size());texts[row.index]=row.key;
        if(!std::strcmp(row.key,"gameplay_ui.ACT_NUMBER"))texts[row.index]="ACT {actNumber}";
        if(!std::strncmp(row.key,"acts.",5))texts[row.index]="ACT TITLE";
    }
    for(unsigned i=0;i<ContentStrings;++i){put(b,stringdir+i*8,unsigned(b.size()));put(b,stringdir+i*8+4,unsigned(texts[i].size()));b.insert(b.end(),texts[i].begin(),texts[i].end());b.push_back(0);}
    for(unsigned i=0;i<AnimationKinds;++i){put(b,clipdir+i*16+4,1,2);put(b,clipdir+i*16+6,33,2);}
    for(unsigned i=0;i<CardKinds;++i)for(unsigned j=0;j<4;++j)put(b,carddir+i*16+j*4,j?i*3+j-1:i);
    for(unsigned i=0;i<U_Count;++i)put(b,uidir+i*4,i==U_CampFlames?A_Count:i);
    put(b,12,unsigned(b.size()));uint32_t hash=2166136261u;for(unsigned i=40;i<b.size();++i){hash^=b[i];hash*=16777619u;}put(b,36,hash);return b;
}
static bool store(const State&s){if(!write_ok||!save(s,disk))return false;disk_exists=true;return true;}
static bool restore(State&s){if(!read_ok){s.gold=9998;s.hp=1;return false;}return disk_exists&&load(s,disk.bytes,disk.size);}
static void snapshot(){CHECK(validate(app.game));CHECK(save(app.game,before));}
static void unchanged(){CHECK(validate(app.game));CHECK(save(app.game,after));CHECK(after.size==before.size&&!std::memcmp(after.bytes,before.bytes,before.size));}
static void advance_ms(unsigned ms){while(ms){const unsigned step=ms>173?173:ms;app.advance(step);ms-=step;}}
static HitRegion hit(HitKind kind,int value,bool enabled=true){
    app.render();const HitRegion*found=nullptr;for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==kind&&app.hits[i].value==value){CHECK(!found);found=&app.hits[i];}
    CHECK(found&&found->enabled==enabled);return *found;
}
static void click(HitKind kind,int value){const auto h=hit(kind,value);app.pointer_move(h.x+h.w/2,h.y+h.h/2);app.pointer_button(true);app.pointer_button(false);}
static void settings_back(){
    CHECK(app.settings_open);const auto h=hit(HitKind::UiAction,int(UiAction::Back));
    CHECK(h.enabled&&h.x==0&&h.y==173&&h.w==30&&h.h==21);
    click(HitKind::UiAction,int(UiAction::Back));CHECK(!app.settings_open);
}
static void capture(const char*name){
    if(!screenshots)return;app.pointer.visible=false;app.render();const std::string path=std::string(screenshots)+"/"+name+".ppm";
    FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");unsigned char row[ScreenW*3];
    for(int y=0;y<ScreenH;++y){for(int x=0;x<ScreenW;++x){const uint16_t p=app.pixels[y*ScreenW+x];row[x*3]=uint8_t((p>>11)*255/31);row[x*3+1]=uint8_t(((p>>5)&63)*255/63);row[x*3+2]=uint8_t((p&31)*255/31);}CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static void new_game(bool fast=false,uint32_t clock=1000){
    disk_exists=false;disk.size=0;read_ok=write_ok=true;app.quit=false;app.fast_mode=false;
    app.map_clock_ms=clock;app.animation=app.tick_remainder=0;app.run_number=0;
    app.selected_character=Character::Ironclad;app.selected_region=reference::ActKind::Overgrowth;app.selected_ascension=0;app.selected_seed[0]=0;
    app.return_to_title();app.forget_run_checkpoint();CHECK(app.title&&!app.has_save);
    if(fast){click(HitKind::Menu,1);click(HitKind::UiAction,int(UiAction::SettingsFastMode));settings_back();}
    click(HitKind::Menu,0);CHECK(app.run_mode.screen==RunMenu::Singleplayer&&!app.character_select);
    click(HitKind::UiAction,int(UiAction::OpenStandardRun));CHECK(app.character_select);app.render();advance_ms(350);click(HitKind::UiAction,int(UiAction::StartCharacter));
    CHECK(!app.title&&app.game.original_events.id==OriginalEventId::Neow&&app.game.world.total_floor==1);
    CHECK(app.fast_mode==fast&&app.map_act_intro.fast_at_construction==fast&&!app.map_act_intro.played);
    CHECK(!app.map_window.visible&&!app.map_act_intro.banner_count);app.pointer.visible=false;
}
static void proceed_neow(){
    CHECK(app.game.original_events.offers[0]==WorldRelicFishingRod);
    app.render();advance_ms(1000); // authored Ancient options enter from below.
    click(HitKind::Choice,0);CHECK(app.game.original_events.finished&&!world_relic_pending(app.game));
    app.render();advance_ms(1000);
    click(HitKind::Choice,0);CHECK(app.game.world.room_open==2&&app.map_window.is_open&&app.panel==3);
    CHECK(app.game.phase==Phase::Event&&app.game.original_events.active);app.pointer.visible=false;
    uint8_t ids[reference::Columns];CHECK(world_available_points(app.game,ids,reference::Columns));
    CHECK(app.cursor==world_current_map(app.game)->points[ids[0]].col); // valid before another render.
}
static uint16_t black_over(uint16_t back,unsigned alpha){
    const unsigned r=((back>>11)*(255-alpha)+127)/255,g=(((back>>5)&63)*(255-alpha)+127)/255,b=((back&31)*(255-alpha)+127)/255;
    return uint16_t(r<<11|g<<5|b);
}
static void banner_pixels(unsigned layers){
    CHECK(app.map_act_intro.banner_count==layers&&layers<=2);app.pointer.visible=false;
    for(unsigned i=0;i<layers;++i)CHECK(app.map_act_intro.banners[i].sample().background_alpha==1024);
    app.map_act_intro.banner_count=0;app.render();std::vector<uint16_t>base(app.pixels,app.pixels+ScreenW*ScreenH);
    app.map_act_intro.banner_count=uint8_t(layers);app.render();unsigned changed=0;
    for(unsigned i=0;i<base.size();++i)if(base[i]!=app.pixels[i]){CHECK(i/ScreenW>=94&&i/ScreenW<140);++changed;}
    CHECK(changed>0);uint16_t expected=base[120*ScreenW+2];for(unsigned i=0;i<layers;++i)expected=black_over(expected,64);
    CHECK(app.pixels[120*ScreenW+2]==expected);unchanged();
}
static void ordinary_intro(){
    new_game();proceed_neow();snapshot();const uint32_t began=app.map_clock_ms;
    CHECK(app.map_act_intro.camera_running&&app.map_act_intro.banner_count==1&&app.map_act_intro.y_q12==2400*4096);
    const int y=app.map_act_intro.y_q12;for(unsigned i=0;i<4;++i)app.render();CHECK(app.map_act_intro.camera_elapsed==0&&app.map_act_intro.y_q12==y);
    app.input(Key::Up);app.input(Key::Right);app.input(Key::Confirm);CHECK(app.map_act_intro.camera_running&&!app.map_scroll&&!app.map_travel.pending);unchanged();
    advance_ms(999);CHECK(app.map_act_intro.camera_elapsed==999&&!app.map_act_intro.property_started&&!app.map_can_scroll());
    app.advance(1);CHECK(app.map_act_intro.property_started&&app.map_act_intro.y_q12==y);
    advance_ms(749);CHECK(!app.map_act_intro.interruptible);app.advance(1);CHECK(app.map_act_intro.interruptible&&app.map_can_scroll());
    CHECK(app.map_act_intro.banners[0].elapsed==1750);banner_pixels(1);capture("map-act-normal-1750");
    // Advancing the production App without rendering still completes both
    // independent lifetimes. Redraw itself never advances their clocks.
    advance_ms(2250);CHECK(!app.map_act_intro.camera_running&&app.map_act_intro.y_q12==0&&app.map_act_intro.interruptible);
    CHECK(app.map_act_intro.banner_count==1);capture("map-act-normal-4000");advance_ms(750);
    CHECK(uint32_t(app.map_clock_ms-began)==4750&&!app.map_act_intro.banner_count);unchanged();
    app.input(Key::Map);advance_ms(350);CHECK(!app.map_window.visible);
    // Repeating retained Event Proceed must reopen without a second banner.
    click(HitKind::Choice,0);CHECK(app.map_window.is_open);
    CHECK(app.map_act_intro.played&&!app.map_act_intro.camera_running&&!app.map_act_intro.banner_count);unchanged();
}
static void top_bar_and_fast(){
    new_game();snapshot();app.input(Key::Map);CHECK(app.map_act_intro.played&&!app.map_act_intro.camera_running&&app.map_act_intro.banner_count==1);
    CHECK(app.map_act_intro.y_q12==0&&!app.map_routes_active());advance_ms(625);unchanged();app.input(Key::Map);advance_ms(350);
    const unsigned elapsed=app.map_act_intro.banners[0].elapsed;CHECK(elapsed==975&&!app.map_window.visible);advance_ms(500);CHECK(app.map_act_intro.banners[0].elapsed==elapsed);
    proceed_neow();snapshot();CHECK(!app.map_act_intro.camera_running&&app.map_act_intro.banner_count==1&&app.map_act_intro.banners[0].elapsed==elapsed);
    advance_ms(4750-elapsed);CHECK(!app.map_act_intro.banner_count);unchanged();
    new_game(true);proceed_neow();snapshot();CHECK(app.map_act_intro.played&&!app.map_act_intro.camera_running&&app.map_act_intro.banner_count==1);
    advance_ms(1750);banner_pixels(1);capture("map-act-fast-1750");advance_ms(1499);CHECK(app.map_act_intro.banner_count==1);
    app.advance(1);CHECK(!app.map_act_intro.banner_count);unchanged();
    // Camera durations are readonly MapScreen-construction values; the
    // branch decision and each banner instead read the current preference.
    new_game(true);app.input(Key::Back);CHECK(app.paused);click(HitKind::Menu,1);
    click(HitKind::UiAction,int(UiAction::SettingsFastMode));settings_back();click(HitKind::Menu,0);
    CHECK(!app.fast_mode&&!app.paused&&app.map_act_intro.fast_at_construction);proceed_neow();snapshot();
    CHECK(app.map_act_intro.camera_running&&app.map_act_intro.delay()==500&&!app.map_act_intro.banners[0].fast);
    advance_ms(874);CHECK(!app.map_act_intro.interruptible);app.advance(1);CHECK(app.map_act_intro.interruptible);
    advance_ms(1125);CHECK(!app.map_act_intro.camera_running&&app.map_act_intro.banner_count==1);unchanged();
}
static void close_pause_and_interrupt(){
    new_game();proceed_neow();snapshot();advance_ms(1250);app.input(Key::Map);
    CHECK(app.map_window.closing()&&!app.map_window.is_open);advance_ms(349);CHECK(app.map_window.visible&&app.map_act_intro.camera_elapsed==1599);
    app.advance(1);CHECK(!app.map_window.visible&&app.map_act_intro.camera_running&&app.map_act_intro.camera_elapsed==1600);
    const int hidden_y=app.map_act_intro.y_q12;advance_ms(750);app.render();
    CHECK(app.map_act_intro.camera_elapsed==1600&&app.map_act_intro.banners[0].elapsed==1600&&app.map_act_intro.y_q12==hidden_y);
    app.input(Key::Map);CHECK(app.map_act_intro.camera_elapsed==1600&&app.map_act_intro.y_q12==0);
    advance_ms(150);CHECK(app.map_act_intro.interruptible&&app.map_act_intro.y_q12>0);app.input(Key::Right);
    CHECK(app.map_act_intro.camera_running);app.input(Key::Up);
    CHECK(!app.map_act_intro.camera_running&&app.map_act_intro.input_guard&&!app.map_scroll&&app.map_act_intro.banner_count==1);
    advance_ms(199);app.input(Key::Up);CHECK(app.map_act_intro.input_guard&&!app.map_scroll);app.advance(1);CHECK(!app.map_act_intro.input_guard);
    app.advance(67);CHECK(!app.map_act_intro.y_q12);unchanged();
    // Immediate Close(false) pauses without consuming a visual close tail.
    new_game();proceed_neow();advance_ms(500);app.close_map_window(false);advance_ms(1000);
    CHECK(!app.map_window.visible&&app.map_act_intro.camera_elapsed==500&&app.map_act_intro.banners[0].elapsed==500);
    app.open_map_window(true);CHECK(app.map_act_intro.camera_running&&app.map_act_intro.y_q12==0);
    advance_ms(500);CHECK(app.map_act_intro.property_started&&app.map_act_intro.camera_from_q12==0);
}
static void move_to_background(){
    app.render();for(int y=70;y<120;y+=7)for(int x=8;x<70;x+=7)if(!app.hit_at(x,y)){app.pointer_move(x,y);return;}
    CHECK(false);
}
static void pointer_and_capstone(){
    new_game();proceed_neow();snapshot();advance_ms(1749);move_to_background();app.pointer_button(true);
    CHECK(app.map_act_intro.camera_running&&!app.pointer.held);app.pointer_button(false);app.advance(1);
    move_to_background();app.pointer_button(true);CHECK(!app.map_act_intro.camera_running&&app.map_act_intro.input_guard&&!app.pointer.held);
    app.pointer_button(false);advance_ms(200);CHECK(!app.map_act_intro.input_guard&&app.map_act_intro.banner_count==1);
    move_to_background();const int x=app.pointer.x,y=app.pointer.y;app.pointer_button(true);CHECK(app.map_dragging&&app.pointer.held);
    app.pointer_move(x,y+50);CHECK(app.map_act_intro.target_q12>0&&app.map_scroll>0);app.advance(33);CHECK(app.map_act_intro.y_q12>0);
    app.pointer_button(false);CHECK(!app.map_dragging&&!app.map_travel.pending);app.input(Key::Left);CHECK(!app.map_scroll);
    app.advance(67);CHECK(!app.map_act_intro.y_q12);unchanged();
    // Capstones cover the map without disabling its process mode.
    new_game();proceed_neow();snapshot();advance_ms(1000);app.input(Key::Inspect);CHECK(app.panel==1&&app.map_window.is_open);
    advance_ms(750);CHECK(app.map_act_intro.camera_elapsed==1750&&app.map_act_intro.banners[0].elapsed==1750);
    app.input(Key::Map);CHECK(app.panel==3&&app.map_act_intro.banner_count==1);unchanged();
}
static void saving_and_reset(){
    constexpr uint32_t intro_start=0xffffff00u;
    // Starting the character screen and both Ancient pages now consumes
    // their actual 350+1000+1000ms entries. Keep the map animation itself
    // crossing uint32 wrap, rather than wrapping during that prelude.
    constexpr unsigned prelude=350+1000+1000;
    new_game(false,intro_start-prelude);proceed_neow();CHECK(app.map_clock_ms==intro_start);snapshot();advance_ms(1250);CHECK(app.save_current_run());
    const unsigned elapsed=app.map_act_intro.camera_elapsed;const int y=app.map_act_intro.y_q12;
    read_ok=false;app.input(Key::Load);CHECK(app.map_window.is_open&&app.map_act_intro.camera_elapsed==elapsed&&app.map_act_intro.y_q12==y);unchanged();
    write_ok=false;CHECK(!app.save_current_run());CHECK(app.map_act_intro.camera_elapsed==elapsed);write_ok=read_ok=true;
    app.input(Key::Load);CHECK(!app.map_window.visible&&app.map_act_intro.played&&!app.map_act_intro.camera_running&&!app.map_act_intro.banner_count);unchanged();
    app.input(Key::Map);CHECK(app.map_window.is_open&&!app.map_window.opening()&&!app.map_act_intro.y_q12&&!app.map_act_intro.banner_count);unchanged();
    new_game(false,intro_start-prelude);proceed_neow();CHECK(app.map_clock_ms==intro_start);snapshot();advance_ms(4750);
    CHECK(app.map_clock_ms==uint32_t(intro_start+4750u)&&!app.map_act_intro.camera_running&&!app.map_act_intro.banner_count);unchanged();
    app.return_to_title();CHECK(!app.map_window.visible&&!app.map_act_intro.banner_count&&!app.map_act_intro.camera_running);
    new_game();proceed_neow();CHECK(app.map_act_intro.camera_running&&app.map_act_intro.camera_elapsed==0);
}
static void finish_routing_fixture(){
    // Supply room outcomes only while routing to the next act. Map generation,
    // room entry, boss transition and the presentation lifecycle remain real.
    if(app.game.original_events.active){
        if(app.game.world.event_combat_parent){app.game.world.room_type=app.game.world.parent_room_type;app.game.world.parent_room_type=app.game.world.event_combat_parent=0;}
        app.game.original_events=OriginalEventState{};app.game.phase=Phase::Event;
    }
    if(world_room_is_combat(app.game))app.game.phase=Phase::Reward;
    if(app.game.phase==Phase::Treasure)app.game.treasure_state=2;
    CHECK(world_finish_room(app.game));CHECK(validate(app.game));
}
static void later_act(){
    new_game();proceed_neow();advance_ms(4750);CHECK(app.map_act_intro.interruptible);
    for(unsigned guard=0;app.game.world.act_index==0&&guard<64;++guard){
        if(!world_can_travel(app.game))finish_routing_fixture();
        uint8_t ids[reference::Columns];const unsigned count=world_available_points(app.game,ids,reference::Columns);CHECK(count);
        CHECK(world_choose_point(app.game,ids[0]));finish_routing_fixture();app.reset_presentation(true);app.sync_map_window();
    }
    CHECK(app.game.world.act_index==1&&app.game.phase==Phase::Map&&app.game.world.current_point==reference::NoPoint);
    CHECK(app.map_window.is_open&&app.map_act_intro.camera_running&&app.map_act_intro.banner_count==2&&app.map_act_intro.interruptible);
    snapshot();hit(HitKind::Command,int(Key::Map),false);app.input(Key::Map);CHECK(app.map_window.is_open);
    // The prior naturally completed camera leaves the source's interruptible
    // latch set. SetMap does not clear it, so this next sweep can cancel early.
    app.input(Key::Up);CHECK(!app.map_act_intro.camera_running&&app.map_act_intro.input_guard&&app.map_act_intro.banner_count==2);
    advance_ms(1750);banner_pixels(2);capture("map-act-second-double-banner");
    CHECK(app.save_current_run());app.input(Key::Load);
    CHECK(app.map_window.is_open&&!app.map_window.opening()&&app.map_act_intro.played&&!app.map_act_intro.camera_running&&!app.map_act_intro.banner_count);
    CHECK(app.map_act_room_entered&&!app.map_act_restore_pending);unchanged();
}
static void replaced_map(){
    new_game();proceed_neow();advance_ms(1800);CHECK(app.map_act_intro.interruptible);app.close_map_window(false);
    const unsigned old=world_current_map(app.game)->config.variant;
    // This isolated acquisition invokes the real modifier/map replacement;
    // the finished Neow page and subsequent repeat Proceed remain live UI.
    CHECK(gain_world_relic(app.game,WorldRelicGoldenCompass));CHECK(world_refresh_map_modifiers(app.game));
    CHECK(world_current_map(app.game)->config.variant!=old);app.sync_map_window();
    CHECK(!app.map_act_intro.played&&app.map_act_intro.interruptible&&app.map_act_intro.camera_running&&app.map_act_intro.banner_count==1);
    snapshot();click(HitKind::Choice,0);
    CHECK(app.map_window.is_open&&app.map_act_intro.played&&app.map_act_intro.camera_elapsed==0&&app.map_act_intro.banner_count==2);
    CHECK(app.map_act_intro.interruptible&&app.map_act_intro.can_scroll());unchanged();
}
int main(int argc,char**argv){
    actual_profile=argc>1;std::vector<uint8_t>bytes;
    if(actual_profile){FILE*f=std::fopen(argv[1],"rb");CHECK(f);bytes.resize(spire_test::pack_size(f));CHECK(std::fread(bytes.data(),1,bytes.size(),f)==bytes.size());CHECK(!std::fclose(f));}
    else bytes=synthetic_profile();if(argc>2)screenshots=argv[2];CHECK(app.init(bytes.data(),bytes.size(),{store,restore}));
    ordinary_intro();top_bar_and_fast();close_pause_and_interrupt();pointer_and_capstone();saving_and_reset();later_act();replaced_map();
    std::printf("world map act intro UI: %u checks passed; %s content; actual new-run input, pause, interruption, load and next act\n",checks,actual_profile?"actual":"synthetic");
}
