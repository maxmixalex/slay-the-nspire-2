// Actual-pack mode checks the normal history/daily render paths against the
// private original-culture oracle. No source font, raster or pack is embedded.
#include "../src/app.cpp"
#include "content_fixture.hpp"
#include "lcd_scene_oracle.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace spire;
static App a;
static unsigned checks,reads;
static uint32_t now=1792847220;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"calendar history:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static uint32_t wall(){return now;}
static bool read_at(void* context,uint32_t offset,void* output,uint32_t size){
    ++reads;const auto& bytes=*static_cast<const std::vector<uint8_t>*>(context);
    if(offset>bytes.size()||size>bytes.size()-offset)return false;
    std::memcpy(output,bytes.data()+offset,size);return true;
}
static std::string unhex(const std::string& hex){
    CHECK(hex.size()%2==0);std::string result;
    for(size_t i=0;i<hex.size();i+=2){char*end=nullptr;const auto part=hex.substr(i,2);const auto n=std::strtoul(part.c_str(),&end,16);CHECK(end&&!*end&&n<256);result+=char(n);}
    return result;
}
static std::string shaped(std::string text,unsigned language){
    if(language!=15)return text;
    // Independent replacement over the immutable grapheme contract; unlike
    // the runtime, this does not compact an overlapping buffer in place.
    for(const auto& cluster:calendar_detail::ThaiMonthClusters){
        const uint32_t cp=cluster.codepoint;
        const char raw[]={char(0xf0|(cp>>18)),char(0x80|((cp>>12)&63)),char(0x80|((cp>>6)&63)),char(0x80|(cp&63))};
        size_t at=0;while((at=text.find(cluster.text,at))!=std::string::npos){text.replace(at,std::strlen(cluster.text),raw,4);at+=4;}
    }
    return text;
}
static void exact_glyphs(const std::string& text,unsigned size,unsigned face){
    const unsigned before=reads;RichReader reader(text.c_str());RichChar ch{};
    while(reader.next(ch))CHECK(a.content.has_exact_glyph(ch.codepoint,size,face));
    CHECK(reads==before); // Availability never decodes/reads any glyph pixels.
}
static void capture(const char* path,const char* name){
    if(!path)return;FILE* file=std::fopen((std::string(path)+"/"+name+".ppm").c_str(),"wb");CHECK(file);
    std::fprintf(file,"P6\n320 240\n255\n");
    for(uint16_t pixel:a.pixels){const unsigned char rgb[]={uint8_t((pixel>>11)*255/31),uint8_t(((pixel>>5)&63)*255/63),uint8_t((pixel&31)*255/31)};CHECK(std::fwrite(rgb,1,3,file)==3);}
    CHECK(!std::fclose(file));
}
static void same_band(const std::vector<uint16_t>& actual,const std::vector<uint16_t>& expected,int top,int bottom){
    unsigned ink=0;
    for(int y=top;y<bottom;++y)for(int x=0;x<320;++x){
        const unsigned i=y*320+x;
        if(actual[i]!=expected[i])std::fprintf(stderr,"calendar pixel %d,%d: %u != %u\n",x,y,actual[i],expected[i]);
        CHECK(actual[i]==expected[i]);ink+=actual[i]!=a.pixels[i];
    }
    CHECK(ink>0);
}
static void scenes(const std::vector<uint8_t>& pack,const std::string& date,const std::string& time,const std::string& daily,const char* captures){
    using namespace spire_test;
    auto& record=a.progress.history[0];record={};record.serial=1;record.character=0;record.started_at=now;
    record.hp=61;record.max_hp=80;record.gold=145;record.floor=2;record.outcome=1;std::strcpy(record.seed_text,"CALENDAR");
    const auto paint_history=[&](bool visible){for(auto& pixel:a.pixels)pixel=scene_rgb(21,40,65);Canvas canvas{a};if(!visible)canvas.text_visible_characters=0;history_header(canvas,record,0,0);};
    paint_history(true);std::vector<uint16_t> actual(a.pixels,a.pixels+320*240);
    paint_history(false);std::vector<uint16_t> expected(a.pixels,a.pixels+320*240);
    std::string label=a.content.world_string("run_history","INFO.DATE_TIME");
    const auto replace=[&](const char* key,const std::string& value){const size_t at=label.find(key);CHECK(at!=std::string::npos);label.replace(at,std::strlen(key),value);};
    replace("{Date}",date);replace("{Time}",time);
    const auto glyphs=lcd_scene_letters(a.content,pack,label,8,0);
    CHECK(lcd_scene_width(glyphs)<=UiHistoryLayout::Width);
    lcd_scene_label(expected,glyphs,UiHistoryLayout::Left,75,UiHistoryLayout::Width,16,8,White,64,0,0,0,false,true);
    same_band(actual,expected,75,91);
    a.progress.count=1;a.progress.next_history=1;a.progress_loaded=true;a.progress.tutorial_flags=0;a.title=true;
    a.open_history();for(unsigned i=0;i<50;++i)a.advance(33);a.render();CHECK(a.history_open&&a.content.healthy());capture(captures,"history");
    a.input(Key::Back);a.ui_action(UiAction::OpenDailyRun);CHECK(a.run_mode.screen==RunMenu::Daily);
    const RunOptions before=a.run_mode.pending;const auto character=a.selected_character;const auto ascension=a.selected_ascension;
    char seed[65];std::strcpy(seed,a.selected_seed);
    const auto paint_daily=[&](bool visible){Canvas canvas{a};if(!visible)canvas.text_visible_characters=0;daily_run_screen(canvas);};
    paint_daily(true);actual.assign(a.pixels,a.pixels+320*240);paint_daily(false);expected.assign(a.pixels,a.pixels+320*240);
    const unsigned face=source_label_face(a.content.original_label_fonts(),5);
    const auto daily_glyphs=lcd_scene_letters(a.content,pack,daily,10,face);
    CHECK(lcd_scene_width(daily_glyphs)<=288);
    lcd_scene_label(expected,daily_glyphs,16,39,288,14,10,rgb(135,206,235),64,0,0,0,true,false);
    same_band(actual,expected,39,53);
    CHECK(!std::memcmp(&before,&a.run_mode.pending,sizeof before)&&character==a.selected_character&&ascension==a.selected_ascension&&!std::strcmp(seed,a.selected_seed));
    CHECK(a.run_mode.pending.daily_day==now/86400);
    a.render();CHECK(a.content.healthy());capture(captures,"daily");
}
int main(int argc,char** argv){
    char date[64]{},time[32]{},daily[64]{};
    progress_date_text(a,0,date,sizeof date,time,sizeof time);CHECK(!std::strcmp(date,"—")&&!std::strcmp(time,"—"));
    daily_date_text(a,0,daily,sizeof daily);CHECK(!std::strcmp(daily,"—"));
    // Missing glyph metadata follows the documented readable legacy fallback.
    progress_date_text(a,now,date,sizeof date,time,sizeof time);CHECK(!std::strcmp(date,"October 24, 2026")&&!std::strcmp(time,"1:07 PM"));
    CHECK(!a.content.has_exact_glyph('?',8)&&!a.content.has_exact_glyph(0x10f000,8));
    if(argc==1){std::printf("calendar UI: %u source-only fallback checks; private packs enable actual glyph/raster integration\n",checks);return 0;}
    CHECK(argc==3||argc==4);std::vector<uint8_t> pack;FILE* file=std::fopen(argv[1],"rb");CHECK(file);pack.resize(spire_test::pack_size(file));CHECK(std::fread(pack.data(),1,pack.size(),file)==pack.size());CHECK(!std::fclose(file));
    Callbacks callbacks{};callbacks.wall_clock=wall;CHECK(a.init({&pack,uint32_t(pack.size()),read_at},callbacks));
    const unsigned language=a.content.language();std::ifstream vectors(argv[2]);CHECK(vectors.good());std::string line,scene_date,scene_time,scene_daily;unsigned rows=0;
    while(std::getline(vectors,line)){
        if(line.empty()||line[0]=='#')continue;std::istringstream input(line);std::vector<std::string> fields;std::string value;
        while(std::getline(input,value,'\t'))fields.push_back(value);CHECK(fields.size()==8);
        if(std::stoul(fields[0])!=language)continue;
        const uint32_t stamp=uint32_t(std::stoull(fields[1]));if(!stamp)continue;
        const std::string expected_date=shaped(unhex(fields[5]),language),expected_time=shaped(unhex(fields[6]),language),expected_daily=unhex(fields[7]);
        exact_glyphs(expected_date,8,0);exact_glyphs(expected_time,8,0);exact_glyphs(expected_daily,10,source_label_face(a.content.original_label_fonts(),5));
        progress_date_text(a,stamp,date,sizeof date,time,sizeof time);daily_date_text(a,stamp,daily,sizeof daily);
        CHECK(expected_date==date&&expected_time==time&&expected_daily==daily);++rows;
        if(stamp==now){scene_date=expected_date;scene_time=expected_time;scene_daily=expected_daily;}
    }
    CHECK(rows>400&&!scene_date.empty());const unsigned before=reads;
    CHECK(!a.content.has_exact_glyph(0x10f100,8)&&!a.content.has_exact_glyph('A',31)&&!a.content.has_exact_glyph('A',8,8));CHECK(reads==before);
    if(language==14){CHECK(a.content.has_exact_glyph(0xa0,8));CHECK(a.content.glyph_metrics(0xa0,8).advance>0);}
    if(language==15){CHECK(scene_date.find("2569")!=std::string::npos&&scene_daily.find("2026")!=std::string::npos&&scene_daily.find("October")!=std::string::npos);}
    scenes(pack,scene_date,scene_time,scene_daily,argc==4?argv[3]:nullptr);
    std::printf("calendar history: %u checks, locale %u, %u real resource/provider rows, history and daily LCD rasters passed\n",checks,language,rows);
}
