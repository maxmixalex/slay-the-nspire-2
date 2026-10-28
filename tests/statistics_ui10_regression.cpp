#include "../src/app.cpp"
#include "content_fixture.hpp"
#include "lcd_scene_oracle.hpp"
#include <cstdio>
#include <cmath>
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
static void mask_group_pixels(){
    // Independent source oracle:128px RGBA8 GradientTexture2D, offsets
    // .125/.15/.975/1 and default bilinear sampling over a240px viewport.
    const auto texel=[](int i){const double t=i<0?0:i>127?1:double(i)/127;
        const double a=t<.125?0:t<.15?(t-.125)/(.15-.125):t<.975?1:(1-t)/(1-.975);
        return int(std::floor(a*255+1e-9));};
    const auto alpha=[&](int y){const double sample=(y+.5)/240*128-.5;const int index=int(std::floor(sample));
        return unsigned(std::lround(texel(index)*(1-(sample-index))+texel(index+1)*(sample-index)));};
    const auto mix=[](uint16_t background,uint16_t foreground,unsigned opacity){uint16_t result=0;
        for(unsigned shift:{0u,5u,11u}){const unsigned limit=shift==5?63:31;
            const unsigned value=(((background>>shift)&limit)*(255-opacity)+((foreground>>shift)&limit)*opacity+127)/255;result|=uint16_t(value<<shift);}return result;};
    for(int y=0;y<240;++y)for(int x=0;x<320;++x)app.pixels[y*320+x]=uint16_t(((x*7+y*11)&31)<<11|((x+y*3)&63)<<5|((x*5+y)&31));
    const std::vector<uint16_t>background(app.pixels,app.pixels+320*240);Canvas screen{app};const StatisticsMask mask(screen);
    Canvas content=screen;content.pixels+=29*320;content.clip_h=211;content.opacity=173;content.rect(23,-29,274,240,scene_rgb(36,59,73));
    content.opacity=111;content.rect(37,-29,241,240,scene_rgb(239,200,81));
    const std::vector<uint16_t>group(app.pixels,app.pixels+320*240);mask.apply(screen);
    unsigned changed=0;for(int y=0;y<240;++y){CHECK(StatisticsMask::alpha(y)==alpha(y));for(int x=0;x<320;++x){const unsigned at=y*320+x;
        CHECK(app.pixels[at]==mix(background[at],group[at],alpha(y)));changed+=app.pixels[at]!=group[at];
        if(y<29)CHECK(app.pixels[at]==background[at]);if(y>=38&&y<232)CHECK(app.pixels[at]==group[at]);}}
    CHECK(changed>0&&alpha(28)==0&&alpha(29)>0&&alpha(37)<255&&alpha(38)==255&&alpha(231)==255&&alpha(232)<255&&alpha(239)==0);
    const int overlap=32*320+50;const unsigned a=alpha(32);
    const auto primitive=mix(mix(background[overlap],scene_rgb(36,59,73),(173*a+127)/255),scene_rgb(239,200,81),(111*a+127)/255);
    CHECK(app.pixels[overlap]!=primitive); // Masking each primitive compounds opacity incorrectly.
}
static void layout_and_input(){
    clear_ui_progress(app.progress);const UiStatisticsLayout empty(app.progress);CHECK(empty.count==8&&empty.character_count==0&&empty.height>240);
    // The supported statistics occupy one readable column. Achievements and
    // Timeline have no local data and no longer reserve placeholder rows.
    CHECK(UiStatisticsLayout::Left==52&&UiStatisticsLayout::Width==236);
    for(unsigned i=0;i<empty.count;++i){const auto p=UiStatisticsLayout::projected(empty.entries[i]);
        CHECK(p.x>=52&&p.x+p.w<=290&&p.h>=50);
        if(i)CHECK(empty.entries[i].x==empty.entries[0].x&&empty.entries[i].y>=empty.entries[i-1].y+empty.entries[i-1].h);}
    app.progress.characters_started=31;for(auto&s:app.progress.characters){s.fastest_win_seconds=60;s.highest_ascension=1;}
    const UiStatisticsLayout full(app.progress);CHECK(full.count==23&&full.character_count==5&&full.height>empty.height);
    const unsigned order[]={0,1,3,4,2};for(unsigned i=0;i<5;++i){CHECK(full.models[i]==order[i]);
        CHECK(full.characters[i].x==UiStatisticsLayout::SourceLeft&&full.characters[i].w==1240&&full.characters[i].h>=900);
        if(i)CHECK(full.characters[i].y>full.characters[i-1].y+full.characters[i-1].h);}
    app.progress.characters[0].fastest_win_seconds=0;app.progress.characters[0].highest_ascension=0;
    const UiStatisticsLayout partial(app.progress);CHECK(partial.count==23&&partial.characters[0].h==full.characters[0].h);
    CHECK(partial.characters[2].y==full.characters[2].y); // Conditional secondary labels do not collapse readable rows.
    app.title=true;app.open_statistics();CHECK(app.statistics_presentation.selected==0);
    app.input(Key::Right);CHECK(app.statistics_presentation.selected==0);app.input(Key::Down);CHECK(app.statistics_presentation.selected==1);
    app.input(Key::Left);CHECK(app.statistics_presentation.selected==1);app.input(Key::Down);CHECK(app.statistics_presentation.selected==2);
    app.input(Key::Up);CHECK(app.statistics_presentation.selected==1);app.input(Key::Up);CHECK(app.statistics_presentation.selected==0);
    app.statistics_presentation.selected=9;app.input(Key::Down);CHECK(app.statistics_presentation.selected==10&&app.progress_page_scroll.target>0);
    app.input(Key::Back);CHECK(!app.statistics_open&&app.compendium_open);unchanged();
}
static void entry_pixels(const std::vector<uint8_t>&pack){
    clear_ui_progress(app.progress);app.progress.characters_started=1;app.progress.characters[0].fastest_win_seconds=59;app.progress.characters[0].highest_ascension=10;
    const UiStatisticsLayout layout(app.progress);app.pointer.visible=true;app.pointer.x=319;app.pointer.y=0;app.map_clock_ms=2000;
    struct Case {unsigned entry;const char*top;const char*bottom;const char*amount;const char*other;uint32_t wins,losses;bool red;};
    const Case cases[]={
        {0,"ENTRY_PLAYTIME.top",nullptr,"","",0,0,false},
        {0,"ENTRY_PLAYTIME.top","ENTRY_PLAYTIME.bottom","","",0,0,false},
        {1,nullptr,"ENTRY_CARDS.bottom","","558/558",0,0,false},
        {2,nullptr,"ENTRY_WIN_LOSS.bottom","","",23,7,false},
        {2,"ENTRY_WIN_LOSS.top","ENTRY_WIN_LOSS.bottom","50/50","",UINT32_MAX,UINT32_MAX,false},
        {3,"ENTRY_MONSTER.top","ENTRY_MONSTER.bottom","98765","42/101",0,0,false},
        {7,"ENTRY_STREAK.top",nullptr,"22","",0,0,false},
        {8,"ENTRY_CHAR_PLAYTIME.top","ENTRY_CHAR_PLAYTIME.bottom","","",0,0,false},
        {9,"ENTRY_CHAR_WIN_LOSS.top","ENTRY_CHAR_WIN_LOSS.bottom","10","",101,9,true},
        {10,"ENTRY_CHAR_STREAK.top","ENTRY_CHAR_STREAK.bottom","9","22",0,0,false}
    };
    for(const auto&test:cases){
        // The readable LCD layout has eight overall entries in one column,
        // then three per played character. Keep independent source constants;
        // the former two-column/4px oracle described the superseded UI10 view.
        const int sy=test.entry<8?312+int(test.entry)*280:2784+int(test.entry-8)*268;
        const int scroll=projected(sy)-56;
        app.progress_page_scroll.position=app.progress_page_scroll.target=scroll*256;
        for(auto&m:app.statistics_presentation.entries)m={};
        ProgressTextValues upper;upper.amount=test.amount;upper.playtime="3:25:20";upper.fastest="00:59";upper.wins=test.wins;upper.losses=test.losses;ProgressTextValues lower=upper;lower.amount=test.other;
        const auto paint=[&](bool visible){for(auto&pixel:app.pixels)pixel=scene_rgb(21,40,65);Canvas page{app};page.pixels+=29*320;page.clip_h=211;if(!visible)page.text_visible_characters=0;
            statistics_entry(page,layout,test.entry,I7_StatsClock,test.top,test.bottom,upper,lower,test.red);};
        paint(true);const std::vector<uint16_t>actual(app.pixels,app.pixels+320*240);paint(false);std::vector<uint16_t>expected(app.pixels,app.pixels+320*240);
        const auto label=[&](const char*key,bool second){if(!key)return;std::string value=text(key);
            const auto sub=[&](const char*variable,const std::string&replacement){if(value.find(variable)!=std::string::npos)replace(value,variable,replacement);};
            sub("{Amount}",second?test.other:test.amount);sub("{Playtime}","3:25:20");sub("{FastestWin}","00:59");sub("{Wins}",std::to_string(test.wins));sub("{Losses}",std::to_string(test.losses));
            constexpr unsigned px=10;auto letters=lcd_scene_letters(app.content,pack,value,px,0);
            const int yy=56+((test.top&&test.bottom)?(second?26:2):10);
            lcd_scene_label(expected,letters,92,yy,187,24,px,test.red&&!second?scene_rgb(255,85,85):scene_rgb(255,246,226),64,0,0,0,false,true);
        };label(test.top,false);label(test.bottom,true);
        unsigned changed=0;for(unsigned i=0;i<expected.size();++i){if(actual[i]!=expected[i])std::fprintf(stderr,"entry=%u pixel=%u,%u actual=%u expected=%u\n",test.entry,i%320,i/320,actual[i],expected[i]);CHECK(actual[i]==expected[i]);changed+=actual[i]!=app.pixels[i];}CHECK(changed>0);
    }unchanged();
}
static void actual_flow(const std::vector<uint8_t>&pack,const char*folder){
    if(pack.empty())return;entry_pixels(pack);clear_ui_progress(app.progress);app.progress.characters_started=31;
    for(unsigned i=0;i<5;++i){auto&s=app.progress.characters[i];s.play_seconds=3723+i*66;s.wins=i+2;s.losses=i+3;s.highest_ascension=uint8_t(i+1);s.fastest_win_seconds=1200+i*100;s.current_streak=i;s.best_streak=i+4;}
    app.pointer.visible=true;app.pointer.x=319;app.pointer.y=0;app.open_statistics();app.map_clock_ms+=400;app.render();
    unsigned entries=0;for(unsigned i=0;i<unsigned(app.hit_count);++i)entries+=app.hits[i].kind==HitKind::StatisticsEntry;CHECK(entries==4);capture(folder,"statistics-source-layout");
    // Fresh fastest/ascension metadata does not create the original hidden rows.
    app.progress.characters[0].fastest_win_seconds=0;app.progress.characters[0].highest_ascension=0;app.render();capture(folder,"statistics-source-hidden-rows");
    app.pointer.visible=false;app.statistics_presentation.selected=1;app.render();app.map_clock_ms+=50;app.render();capture(folder,"statistics-source-playtime-hover");
    app.statistics_presentation.selected=2;app.render();app.map_clock_ms+=50;app.render();capture(folder,"statistics-source-ascension-hover");
    app.statistics_presentation.selected=21;app.input(Key::Down);CHECK(app.statistics_presentation.selected==22&&app.progress_page_scroll.target>0);app.progress_page_scroll.position=app.progress_page_scroll.target;app.render();capture(folder,"statistics-source-last-character");
    app.pointer.visible=true;app.pointer.x=309;app.pointer.y=135;app.render();const auto*thumb=app.hit_at(309,135);CHECK(thumb&&thumb->kind==HitKind::ProgressThumb);
    app.pointer_button(true);app.pointer_move(309,216);CHECK(app.progress_page_scroll.target==app.progress_page_scroll.maximum);app.pointer_button(false);CHECK(!app.progress_page_scroll.dragging);
    unchanged();
}
int main(int argc,char**argv){
    std::vector<uint8_t>pack;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);pack.resize(spire_test::pack_size(f));CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());std::fclose(f);CHECK(app.init(pack.data(),pack.size(),{}));}
    new_run(app.game,0x1735,false,Character::Ironclad);CHECK(save(app.game,before));mask_group_pixels();layout_and_input();actual_flow(pack,argc>2?argv[2]:nullptr);
    std::puts("Statistics: original container geometry, conditional rows, locale fonts/colors, focus/hover/scroll and canonical purity passed");
}
