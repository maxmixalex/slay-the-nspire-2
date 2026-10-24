// Compile this unchanged driver against the frozen baseline and candidate.
// The private pack is opened read-only through the production range callback.
// These are unpaced host CPU intervals, not calculator or browser display FPS.
#include "app.hpp"
#include "resource_file_cursor.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace spire;
using Clock=std::chrono::steady_clock;
namespace {
static App app;
static SaveData before,after;
static unsigned save_attempts;
struct Io {uint64_t calls=0,bytes=0,seeks=0;};
struct Source {
    FILE* file;uint32_t length;ResourceFileCursor cursor{};Io io{};
    FILE* trace=nullptr;const char* scene="",*phase="";
};
static int seek_file(void*file,long offset,int origin){return std::fseek(static_cast<FILE*>(file),offset,origin);}
static unsigned read_file(void*out,unsigned size,unsigned count,void*file){return unsigned(std::fread(out,size,count,static_cast<FILE*>(file)));}
static bool read_resource(void*context,uint32_t offset,void*out,uint32_t count){
    auto&s=*static_cast<Source*>(context);bool sought=false;
    const bool good=offset<=s.length&&count<=s.length-offset&&s.cursor.read(s.file,offset,out,count,seek_file,read_file,sought);
    ++s.io.calls;s.io.bytes+=count;s.io.seeks+=sought;
    if(s.trace)std::fprintf(s.trace,"{\"scene\":\"%s\",\"phase\":\"%s\",\"offset\":%u,\"bytes\":%u,\"seek\":%s,\"ok\":%s}\n",
        s.scene,s.phase,offset,count,sought?"true":"false",good?"true":"false");
    return good;
}
static bool save_game(const State&){++save_attempts;return false;}
static double milliseconds(Clock::time_point begin){return std::chrono::duration<double,std::milli>(Clock::now()-begin).count();}
static uint32_t framebuffer_hash(){uint32_t hash=2166136261u;for(uint16_t p:app.pixels){hash=(hash^uint8_t(p))*16777619u;hash=(hash^uint8_t(p>>8))*16777619u;}return hash;}
struct Mark {Clock::time_point time;Io io;};
static Mark mark(const Source&s){return {Clock::now(),s.io};}
struct Cost {
    unsigned samples=0;double elapsed=0,maximum=0;Io io{};
    void add(const Mark&start,const Source&s){const double duration=milliseconds(start.time);++samples;elapsed+=duration;if(duration>maximum)maximum=duration;
        io.calls+=s.io.calls-start.io.calls;io.bytes+=s.io.bytes-start.io.bytes;io.seeks+=s.io.seeks-start.io.seeks;}
    void write(const char*scene,const char*phase)const{
        std::printf("{\"operation\":\"stage\",\"scene\":\"%s\",\"phase\":\"%s\",\"samples\":%u,\"cpu_elapsed_ms\":%.3f,\"mean_ms\":%.3f,\"maximum_ms\":%.3f,\"source_calls\":%llu,\"source_bytes\":%llu,\"source_seeks\":%llu}\n",
            scene,phase,samples,elapsed,samples?elapsed/samples:0,maximum,(unsigned long long)io.calls,(unsigned long long)io.bytes,(unsigned long long)io.seeks);
    }
};
static bool choice_fixture(){
    // A seeded practice run already owns Secret Technique. Put that existing
    // instance in the opening hand, preserving both zones, then use real play()
    // to suspend on its draw-pile choice. No effects or choice state are forged.
    auto&s=app.game;int at=-1;
    for(int i=0;i<s.draw.count;++i)if(s.cards[s.draw.ids[i]].kind==SecretTechnique){at=i;break;}
    if(at<0||!s.hand.count)return false;
    const int hand=s.hand.count-1;const uint16_t chosen=s.draw.ids[at],displaced=s.hand.ids[hand];
    s.hand.ids[hand]=chosen;s.draw.ids[at]=displaced;s.zones[chosen]=Zone::Hand;s.zones[displaced]=Zone::Draw;
    return play(s,hand,0)&&s.phase==Phase::ExtendedChoice&&s.expansion.choice==XChoice::Draw&&choice_pile(s).count>=10;
}
static bool setup(unsigned scene){
    new_run(app.game,781,true,Character::Ironclad);app.reset_presentation();
    app.title=scene==0||scene==3;app.library=app.title;app.character_select=false;app.pointer.visible=false;
    app.library_tab=app.library_filter=app.library_page=app.library_selected=app.library_sort=0;
    app.panel=0;app.cursor=0;app.detail_card=app.detail_item=-1;
    if(scene==1){app.game.phase=Phase::Shop;return buy(app.game,ShopRemove)&&app.game.deck_count>=10;}
    if(scene==2)return choice_fixture();
    if(scene==3){
        CardKind ids[CardKinds];const unsigned count=app.library_items(ids);
        for(unsigned i=0;i<count;++i)if(ids[i]==DefectRainbow){app.library_selected=int(i);app.library_page=int(i/8);return true;}
        return false;
    }
    return app.library_items()>=8;
}
static bool capture(const char*directory,const char*scene){
    if(!directory)return true;char path[1024];if(std::snprintf(path,sizeof path,"%s/%s.ppm",directory,scene)>=int(sizeof path))return false;
    FILE*f=std::fopen(path,"wb");if(!f)return false;bool good=std::fprintf(f,"P6\n320 240\n255\n")>0;
    uint8_t row[960];for(unsigned y=0;y<240&&good;++y){for(unsigned x=0;x<320;++x){const uint16_t p=app.pixels[y*320+x];
        row[x*3]=uint8_t((p>>11)*255/31);row[x*3+1]=uint8_t(((p>>5)&63)*255/63);row[x*3+2]=uint8_t((p&31)*255/31);}good=std::fwrite(row,1,sizeof row,f)==sizeof row;}
    return std::fclose(f)==0&&good;
}
static uint32_t save_hash(const SaveData&data){uint32_t hash=2166136261u;for(unsigned i=0;i<data.size;++i)hash=(hash^data.bytes[i])*16777619u;return hash;}
static bool shop_inventory(Source&source,unsigned frames,bool hovered,const char*directory){
    const char*name=hovered?"shop_inventory_hover_7":"shop_inventory_idle_7";
    // The left edge below the top bar belongs to the starter relic. Keep the
    // idle pointer in actual empty space instead of timing its hover tooltip.
    constexpr int idle_x=310,idle_y=35;
    source.scene=name;source.phase="room_open_and_fixture";Cost room,first,warm;auto started=mark(source);
    if(!app.init({&source,source.length,read_resource,nullptr},{save_game,nullptr}))return false;
    new_run(app.game,781,false,Character::Ironclad);app.game.map[0].room=Room::Shop;
    if(!choose_node(app.game,0))return false;
    app.animation=app.map_clock_ms=app.tick_remainder=0;app.reset_presentation();
    app.title=app.paused=app.help=app.library=app.character_select=false;
    app.pointer_move(idle_x,idle_y);app.render();room.add(started,source);
    if(const auto*hit=app.hit_at(idle_x,idle_y);hit&&hit->enabled)return false;
    HitRegion open{};bool found=false;
    for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::UiAction&&app.hits[i].value==int(UiAction::OpenShop)){
        open=app.hits[i];found=true;break;}
    if(!found||app.shop_open||app.game.phase!=Phase::Shop||!save(app.game,before))return false;
    source.phase=hovered?"input_to_first_complete_hover_frame":"input_to_first_complete_inventory_frame";started=mark(source);
    app.pointer_move(open.x+open.w/2,open.y+open.h/2);app.pointer_button(true);app.pointer_button(false);
    app.pointer_move(idle_x,idle_y);app.render();
    if(const auto*hit=app.hit_at(idle_x,idle_y);hit&&hit->enabled)return false;
    if(hovered){
        HitRegion card{};found=false;
        for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::Choice&&app.hits[i].value==0){card=app.hits[i];found=true;break;}
        if(!found)return false;
        // The complete first-open inventory render above belongs to this
        // cold sample too; searching its real hit geometry is not a preload.
        app.pointer_move(card.x+card.w/2,card.y+card.h/3);app.render();
    }
    first.add(started,source);const uint32_t first_pixels=framebuffer_hash();
    if(!app.shop_open||app.game.phase!=Phase::Shop||!app.content.healthy()||app.combat_preparation.active||
       !save(app.game,after)||before.size!=after.size||std::memcmp(before.bytes,after.bytes,before.size))return false;
    source.phase="settle";for(unsigned i=0;i<10;++i){app.advance(250);app.render();}
    unsigned cards=0;for(int i=0;i<app.hit_count;++i)cards+=app.hits[i].kind==HitKind::Choice&&app.hits[i].value>=0&&app.hits[i].value<ShopCardCount;
    if(cards!=7||!capture(directory,name))return false;
    const uint32_t settled_pixels=framebuffer_hash();
    source.phase="warm_inventory_animation_window";
    for(unsigned i=0;i<frames;++i){started=mark(source);app.advance(33);app.render();warm.add(started,source);
        if(!app.shop_open||app.game.phase!=Phase::Shop||!app.content.healthy()||app.combat_preparation.active)return false;}
    const bool unchanged=save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size)&&!save_attempts;
    room.write(name,"room_open_and_fixture");first.write(name,hovered?"input_to_first_complete_hover_frame":"input_to_first_complete_inventory_frame");
    warm.write(name,"warm_inventory_animation_window");
    std::printf("{\"operation\":\"shop_observation\",\"scene\":\"%s\",\"visible_cards\":%u,\"cold_render_calls\":%u,\"hovered\":%s,\"first_rgb565_fnv1a\":%u,\"settled_rgb565_fnv1a\":%u,\"last_rgb565_fnv1a\":%u,\"canonical_before_fnv1a\":%u,\"canonical_after_fnv1a\":%u,\"canonical_state_unchanged\":%s,\"save_callback_attempts\":%u}\n",
        name,cards,hovered?2u:1u,hovered?"true":"false",first_pixels,settled_pixels,framebuffer_hash(),save_hash(before),save_hash(after),unchanged?"true":"false",save_attempts);
    std::fflush(stdout);return unchanged;
}
}
int main(int argc,char**argv){
    if(argc<2||argc>4){std::fprintf(stderr,"Usage: card_grid_benchmark REAL_PACK [FRAMES] [CAPTURE_DIRECTORY]\n");return 2;}
    unsigned frames=180;if(argc>=3){char*end=nullptr;const unsigned long value=std::strtoul(argv[2],&end,10);if(!argv[2][0]||*end||value<10||value>3600)return 2;frames=unsigned(value);}
    FILE*file=std::fopen(argv[1],"rb");if(!file)return 3;if(std::fseek(file,0,SEEK_END)){std::fclose(file);return 3;}
    const long length=std::ftell(file);if(length<40||uint64_t(length)>MaxContentFile){std::fclose(file);return 3;}
    Source source{file,uint32_t(length)};
    // Optional offset-only trace lets the same real-pack driver distinguish
    // card artwork, glyph and string I/O without logging private payloads or
    // changing production cache behavior. Keep tracing off for CPU timing.
    if(const char* path=std::getenv("NSPIRE_RESOURCE_TRACE")){
        source.trace=std::fopen(path,"wb");if(!source.trace){std::fclose(file);return 3;}
    }
    std::printf("{\"operation\":\"scope\",\"pack_bytes\":%u,\"frames_per_scene\":%u,\"elapsed_ms_per_update\":33,\"method\":\"Host steady-clock CPU intervals without pacing; production ranged FILE source; seeded practice fixtures; animations advance on every observed frame.\"}\n",source.length,frames);
    const char*names[]={"library_8","remove_10","draw_choice_10","library_rainbow_8"};
    for(unsigned scene=0;scene<4;++scene){
        source.scene=names[scene];source.phase="open_and_fixture";
        Cost opened,preparation,first,warm;auto started=mark(source);
        if(!app.init({&source,source.length,read_resource,nullptr},{save_game,nullptr})||!setup(scene)){std::fclose(file);return 4;}
        opened.add(started,source);unsigned preparation_updates=0;source.phase="initial_render";started=mark(source);app.render();
        if(app.combat_preparation.active){
            source.phase="preparation";
            preparation.add(started,source);
            while(app.combat_preparation.active){if(++preparation_updates>1024){std::fclose(file);return 5;}
                started=mark(source);app.advance(33);if(app.combat_preparation.active)app.render();preparation.add(started,source);}
            source.phase="first_visible";started=mark(source);app.render();
        }
        first.add(started,source);
        // Establish the settled UI before the window; these updates are
        // explicitly excluded from the measured first-visible and warm costs.
        source.phase="settle";for(unsigned i=0;i<10;++i){app.advance(250);app.render();}
        if(!app.content.healthy()||!save(app.game,before)||!capture(argc==4?argv[3]:nullptr,names[scene])){std::fclose(file);return 6;}
        const auto phase=app.game.phase;const uint32_t initial_hash=framebuffer_hash();
        source.phase="warm";for(unsigned frame=0;frame<frames;++frame){started=mark(source);app.advance(33);app.render();warm.add(started,source);
            if(!app.content.healthy()||app.combat_preparation.active||app.game.phase!=phase){std::fclose(file);return 7;}}
        const bool unchanged=save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size)&&!save_attempts;
        opened.write(names[scene],"open_and_fixture");preparation.write(names[scene],"preparation");first.write(names[scene],"first_visible");warm.write(names[scene],"warm_animation_window");
        std::printf("{\"operation\":\"observation\",\"scene\":\"%s\",\"visible_cards\":%u,\"preparation_updates\":%u,\"first_rgb565_fnv1a\":%u,\"last_rgb565_fnv1a\":%u,\"canonical_state_unchanged\":%s,\"save_callback_attempts\":%u}\n",
            names[scene],scene==1||scene==2?10u:8u,preparation_updates,initial_hash,framebuffer_hash(),unchanged?"true":"false",save_attempts);
        std::fflush(stdout);if(!unchanged){std::fclose(file);return 8;}
        // Use the same ordinary detail command from each visible page. The
        // first sample includes input, any decoding and the complete frame;
        // it cannot hide a first-open stall in preparation or preload time.
        char detail_name[80];std::snprintf(detail_name,sizeof detail_name,"%s_detail",names[scene]);
        source.scene=detail_name;source.phase="input_to_first_complete_frame";
        Cost detail_first,detail_warm;started=mark(source);app.input(Key::Detail);
        // A visible pointer outside the text reproduces normal modal use;
        // metadata-only hit testing must not load offscreen paragraph glyphs.
        app.pointer_move(308,210);app.render();detail_first.add(started,source);
        if(app.detail_card<0||!app.content.healthy()||app.combat_preparation.active||!capture(argc==4?argv[3]:nullptr,detail_name)){std::fclose(file);return 10;}
        const uint32_t detail_hash=framebuffer_hash();source.phase="warm_detail_animation_window";
        for(unsigned frame=0;frame<frames;++frame){started=mark(source);app.advance(33);app.render();detail_warm.add(started,source);
            if(app.detail_card<0||!app.content.healthy()||app.combat_preparation.active||app.game.phase!=phase){std::fclose(file);return 11;}}
        const bool detail_unchanged=save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size)&&!save_attempts;
        detail_first.write(detail_name,"input_to_first_complete_frame");detail_warm.write(detail_name,"warm_detail_animation_window");
        std::printf("{\"operation\":\"detail_observation\",\"scene\":\"%s\",\"first_rgb565_fnv1a\":%u,\"last_rgb565_fnv1a\":%u,\"canonical_state_unchanged\":%s,\"save_callback_attempts\":%u}\n",
            detail_name,detail_hash,framebuffer_hash(),detail_unchanged?"true":"false",save_attempts);
        std::fflush(stdout);if(!detail_unchanged){std::fclose(file);return 12;}
        if(scene==3){
            // Visit all 8 x 59 pointer positions, including blank rows and
            // text after the first gold word. Include the complete sweep and
            // final tooltip frame in the first cost; no glyphs are warmed out
            // of band by a preliminary search. Keep the first real gold word
            // as a stable tooltip owner in both localized packs.
            Cost hover_first,hover_warm;unsigned candidates=0,matches=0;int hover_x=-1,hover_y=-1;
            source.phase="pointer_scan_to_keyword_frame";started=mark(source);
            for(int row=0;row<8;++row)for(int x=123;x<300;x+=3){
                const int y=66+row*15+7;app.pointer_move(x,y);++candidates;CardHoverTarget owner{};
                if(card_hover_target(app,owner)&&owner.detail&&owner.owner.x==123){++matches;if(hover_x<0){hover_x=x;hover_y=y;}}
            }
            if(hover_x<0){std::fclose(file);return 13;}
            app.pointer_move(hover_x,hover_y);app.render();hover_first.add(started,source);
            if(!app.content.healthy()||!app.keyword_hover.owner||!capture(argc==4?argv[3]:nullptr,"rainbow_detail_keyword")){std::fclose(file);return 14;}
            const uint32_t hover_hash=framebuffer_hash();source.phase="warm_keyword_pointer_window";
            for(unsigned frame=0;frame<frames;++frame){started=mark(source);app.pointer_move(hover_x+int(frame&1),hover_y);app.advance(33);app.render();hover_warm.add(started,source);
                if(!app.content.healthy()||!app.keyword_hover.owner||app.detail_card<0||app.game.phase!=phase){std::fclose(file);return 15;}}
            const bool hover_unchanged=save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size)&&!save_attempts;
            hover_first.write(detail_name,"pointer_scan_to_keyword_frame");hover_warm.write(detail_name,"warm_keyword_pointer_window");
            std::printf("{\"operation\":\"keyword_observation\",\"scene\":\"%s\",\"pointer_candidates\":%u,\"pointer_matches\":%u,\"hover_x\":%d,\"hover_y\":%d,\"first_rgb565_fnv1a\":%u,\"last_rgb565_fnv1a\":%u,\"canonical_state_unchanged\":%s}\n",
                detail_name,candidates,matches,hover_x,hover_y,hover_hash,framebuffer_hash(),hover_unchanged?"true":"false");
            std::fflush(stdout);if(!hover_unchanged){std::fclose(file);return 16;}
        }
    }
    // These are the actual merchant rug and its seven visible faces. The
    // settled window keeps room actors animating, exercising exact-background
    // cache validity instead of repeatedly painting a static empty surface.
    for(unsigned hovered=0;hovered<2;++hovered)if(!shop_inventory(source,frames,hovered!=0,argc==4?argv[3]:nullptr)){
        if(source.trace)std::fclose(source.trace);std::fclose(file);return 17+int(hovered);}
    const bool trace_good=!source.trace||std::fclose(source.trace)==0;
    return std::fclose(file)==0&&trace_good?0:9;
}
