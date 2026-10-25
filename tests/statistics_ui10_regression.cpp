#include "../src/app.cpp"
#include "content_fixture.hpp"
#include "source_scene_oracle.hpp"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
using namespace spire_test;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"statistics line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static App app;
static SaveData before,after;
static int projected(int n){return (n*4+10)/21;}
static std::string text(const char*key){return app.content.world_string("stats_screen",key);}
static void replace(std::string&value,const char*key,const std::string&replacement){const auto at=value.find(key);CHECK(at!=std::string::npos);value.replace(at,std::strlen(key),replacement);}
static void unchanged(){CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static void capture(const char*folder,const char*name){if(!folder)return;const std::string path=std::string(folder)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);
    std::fprintf(f,"P6\n320 240\n255\n");for(auto p:app.pixels){const uint8_t rgb[]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}CHECK(!std::fclose(f));}
static void layout_and_input(){
    clear_ui_progress(app.progress);const UiStatisticsLayout empty(app.progress);CHECK(empty.count==9&&empty.character_count==0&&empty.height==240);
    CHECK(empty.entries[0].x==251&&empty.entries[0].y==276&&empty.entries[0].w==500&&empty.entries[0].h==92);
    CHECK(empty.entries[1].x==807&&empty.entries[8].y==644&&empty.entries[8].h==88);
    app.progress.characters_started=31;for(auto&s:app.progress.characters){s.fastest_win_seconds=60;s.highest_ascension=1;}
    const UiStatisticsLayout full(app.progress);CHECK(full.count==24&&full.character_count==5&&full.height==392);
    const unsigned order[]={0,1,3,4,2};for(unsigned i=0;i<5;++i){CHECK(full.models[i]==order[i]);CHECK(full.characters[i].x==227+int(i%2)*564&&full.characters[i].y==836+int(i/2)*384&&full.characters[i].h==352);}
    app.progress.characters[0].fastest_win_seconds=0;app.progress.characters[0].highest_ascension=0;
    const UiStatisticsLayout partial(app.progress);CHECK(partial.characters[0].h==344&&partial.entries[9].h==88&&partial.entries[10].h==88&&partial.entries[11].h==92);
    CHECK(partial.characters[2].y==1220); // The taller right column sets the row height.
    app.title=true;app.open_statistics();CHECK(app.statistics_presentation.selected==0);
    app.input(Key::Right);CHECK(app.statistics_presentation.selected==1);app.input(Key::Down);CHECK(app.statistics_presentation.selected==3);
    app.input(Key::Left);CHECK(app.statistics_presentation.selected==2);app.input(Key::Down);CHECK(app.statistics_presentation.selected==4);
    app.input(Key::Up);CHECK(app.statistics_presentation.selected==2);app.input(Key::Up);CHECK(app.statistics_presentation.selected==0);
    app.statistics_presentation.selected=9;app.input(Key::Down);CHECK(app.statistics_presentation.selected==10&&app.progress_page_scroll.target>0);
    app.input(Key::Back);CHECK(!app.statistics_open&&app.compendium_open);unchanged();
}
static void entry_pixels(const std::vector<uint8_t>&pack){
    clear_ui_progress(app.progress);app.progress.characters_started=1;app.progress.characters[0].fastest_win_seconds=59;app.progress.characters[0].highest_ascension=10;
    const UiStatisticsLayout layout(app.progress);app.pointer.visible=true;app.pointer.x=319;app.pointer.y=0;app.map_clock_ms=2000;
    struct Case {unsigned entry;const char*top;const char*bottom;const char*amount;const char*other;uint32_t wins,losses;bool red;};
    const Case cases[]={
        {0,"ENTRY_ACHIEVEMENTS.top","ENTRY_ACHIEVEMENTS.bottom","—","—",0,0,false},
        {1,"ENTRY_PLAYTIME.top",nullptr,"","",0,0,false},
        {1,"ENTRY_PLAYTIME.top","ENTRY_PLAYTIME.bottom","","",0,0,false},
        {3,nullptr,"ENTRY_WIN_LOSS.bottom","","",23,7,false},
        {3,"ENTRY_WIN_LOSS.top","ENTRY_WIN_LOSS.bottom","50/50","",UINT32_MAX,UINT32_MAX,false},
        {4,"ENTRY_MONSTER.top","ENTRY_MONSTER.bottom","98765","42/101",0,0,false},
        {8,"ENTRY_STREAK.top",nullptr,"22","",0,0,false},
        {9,"ENTRY_CHAR_PLAYTIME.top","ENTRY_CHAR_PLAYTIME.bottom","","",0,0,false},
        {10,"ENTRY_CHAR_WIN_LOSS.top","ENTRY_CHAR_WIN_LOSS.bottom","10","",101,9,true},
        {11,"ENTRY_CHAR_STREAK.top","ENTRY_CHAR_STREAK.bottom","9","22",0,0,false}
    };
    for(const auto&test:cases){
        const unsigned row=test.entry<9?test.entry/2:0;const int sx=test.entry<9?(test.entry%2?807:251):251;
        const int sy=test.entry<9?276+int(row)*92:900+int(test.entry-9)*92,sh=test.entry==8?88:92;
        app.progress_page_scroll.position=app.progress_page_scroll.target=test.entry>=9?120*256:0;const int scroll=test.entry>=9?120:0;
        for(auto&m:app.statistics_presentation.entries)m={};
        ProgressTextValues upper;upper.amount=test.amount;upper.playtime="3:25:20";upper.fastest="00:59";upper.wins=test.wins;upper.losses=test.losses;ProgressTextValues lower=upper;lower.amount=test.other;
        const auto paint=[&](bool visible){for(auto&pixel:app.pixels)pixel=scene_rgb(21,40,65);Canvas page{app};page.pixels+=30*320;page.clip_h=210;if(!visible)page.text_visible_characters=0;
            statistics_entry(page,layout,test.entry,I7_StatsClock,test.top,test.bottom,upper,lower,test.red);};
        paint(true);const std::vector<uint16_t>actual(app.pixels,app.pixels+320*240);paint(false);std::vector<uint16_t>expected(app.pixels,app.pixels+320*240);
        const auto label=[&](const char*key,bool second){if(!key)return;std::string value=text(key);
            const auto sub=[&](const char*variable,const std::string&replacement){if(value.find(variable)!=std::string::npos)replace(value,variable,replacement);};
            sub("{Amount}",second?test.other:test.amount);sub("{Playtime}","3:25:20");sub("{FastestWin}","00:59");sub("{Wins}",std::to_string(test.wins));sub("{Losses}",std::to_string(test.losses));
            unsigned px=5;auto letters=scene_letters(app.content,pack,value,px,0);if(scene_width(letters)>77){px=4;letters=scene_letters(app.content,pack,value,px,0);}
            // The source's22px minimum maps to4px. Two saturated32-bit
            // counters may still exceed the Label; the independent clipped
            // oracle below must match instead of assuming they always fit.
            CHECK(scene_width(letters)<=77||(test.wins==UINT32_MAX&&test.losses==UINT32_MAX));
            const int yy=projected(sy+((test.top&&test.bottom)?(second?36:8):(sh-48)/2))-scroll;
            scene_label(expected,letters,projected(sx+96),yy,77,9,px,test.red&&!second?scene_rgb(255,85,85):scene_rgb(255,246,226),64,0,0,0,false,false,255,0,0,4096,true);
        };label(test.top,false);label(test.bottom,true);
        unsigned changed=0;for(unsigned i=0;i<expected.size();++i){if(actual[i]!=expected[i])std::fprintf(stderr,"entry=%u pixel=%u,%u actual=%u expected=%u\n",test.entry,i%320,i/320,actual[i],expected[i]);CHECK(actual[i]==expected[i]);changed+=actual[i]!=app.pixels[i];}CHECK(changed>0);
    }unchanged();
}
static void actual_flow(const std::vector<uint8_t>&pack,const char*folder){
    if(pack.empty())return;entry_pixels(pack);clear_ui_progress(app.progress);app.progress.characters_started=31;
    for(unsigned i=0;i<5;++i){auto&s=app.progress.characters[i];s.play_seconds=3723+i*66;s.wins=i+2;s.losses=i+3;s.highest_ascension=uint8_t(i+1);s.fastest_win_seconds=1200+i*100;s.current_streak=i;s.best_streak=i+4;}
    app.pointer.visible=true;app.pointer.x=319;app.pointer.y=0;app.open_statistics();app.map_clock_ms+=400;app.render();
    unsigned entries=0;for(unsigned i=0;i<unsigned(app.hit_count);++i)entries+=app.hits[i].kind==HitKind::StatisticsEntry;CHECK(entries>=15);capture(folder,"statistics-source-layout");
    // Fresh fastest/ascension metadata does not create the original hidden rows.
    app.progress.characters[0].fastest_win_seconds=0;app.progress.characters[0].highest_ascension=0;app.render();capture(folder,"statistics-source-hidden-rows");
    app.pointer.visible=false;app.statistics_presentation.selected=1;app.render();app.map_clock_ms+=50;app.render();capture(folder,"statistics-source-playtime-hover");
    app.statistics_presentation.selected=3;app.render();app.map_clock_ms+=50;app.render();capture(folder,"statistics-source-ascension-hover");
    app.statistics_presentation.selected=22;app.input(Key::Down);CHECK(app.statistics_presentation.selected==23&&app.progress_page_scroll.target>0);app.progress_page_scroll.position=app.progress_page_scroll.target;app.render();capture(folder,"statistics-source-last-character");
    app.pointer.visible=true;app.pointer.x=272;app.pointer.y=135;app.render();const auto*thumb=app.hit_at(272,135);CHECK(thumb&&thumb->kind==HitKind::ProgressThumb);
    app.pointer_button(true);app.pointer_move(272,211);CHECK(app.progress_page_scroll.target==app.progress_page_scroll.maximum);app.pointer_button(false);CHECK(!app.progress_page_scroll.dragging);
    unchanged();
}
int main(int argc,char**argv){
    std::vector<uint8_t>pack;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);pack.resize(spire_test::pack_size(f));CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());std::fclose(f);CHECK(app.init(pack.data(),pack.size(),{}));}
    new_run(app.game,0x1735,false,Character::Ironclad);CHECK(save(app.game,before));layout_and_input();actual_flow(pack,argc>2?argv[2]:nullptr);
    std::puts("Statistics: original container geometry, conditional rows, locale fonts/colors, focus/hover/scroll and canonical purity passed");
}
