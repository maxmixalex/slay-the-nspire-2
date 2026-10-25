#include "../src/app.cpp"
#include "content_fixture.hpp"
#include "world_run.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"profiles line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static App app;
static SaveData runs[UiProfileCount],before,after;
static bool has_run[UiProfileCount]{};
static UiPreferences preferences[UiProfileCount]{};
static std::vector<uint8_t> histories[UiProfileCount];
static unsigned active=1,requested=0;
static bool erase_requested=false,storage_ok=true;
static uint8_t staging[UiProgressCapacity];
static unsigned current(){return active;}
static bool read_run(State&s){return has_run[active-1]&&load(s,runs[active-1].bytes,runs[active-1].size);}
static bool write_run(const State&s){if(!storage_ok)return false;has_run[active-1]=save(s,runs[active-1]);return has_run[active-1];}
static bool clear_run(){if(!storage_ok)return false;has_run[active-1]=false;return true;}
static bool read_preferences(UiPreferences& p){p=preferences[active-1];return true;}
static bool write_preferences(const UiPreferences& p){if(!storage_ok)return false;preferences[active-1]=p;return true;}
static bool read_progress(UiProgress&p){const auto&b=histories[active-1];return decode_ui_progress(b.data(),b.size(),p);}
static bool write_progress(const UiProgress&p){if(!storage_ok)return false;const size_t n=encode_ui_progress(p,staging,sizeof staging);if(!n)return false;histories[active-1].assign(staging,staging+n);return true;}
static bool info(unsigned id,UiProgressSummary&out){
    if(id<1||id>UiProfileCount)return false;const auto&b=histories[id-1];
    if(decode_ui_progress_summary(b.data(),b.size(),out))return true;
    out={};out.exists=has_run[id-1];return out.exists;
}
static bool request(unsigned id,bool erase){if(!storage_ok||requested||id<1||id>3)return false;requested=id;erase_requested=erase;return true;}
static void service(bool success=true){
    if(!requested)return;const unsigned id=requested;requested=0;
    if(success){if(erase_requested){histories[id-1].clear();has_run[id-1]=false;preferences[id-1]={};}else active=id;}
    app.finish_profile_change(success);
}
static uint32_t wall(){return 1792838400;}
static bool identical(const SaveData&a,const SaveData&b){return a.size==b.size&&!std::memcmp(a.bytes,b.bytes,a.size);}
// Read exact scene glyph keys and independently composite their alpha masks.
// The oracle never calls the production profile text/fit/transform helpers.
static void scene_check(bool value,int line){if(!value){std::fprintf(stderr,"profile scene line %d\n",line);std::exit(1);}}
#define SCENE(x) scene_check(bool(x),__LINE__)
#include "source_scene_oracle.hpp"
using namespace spire_test;
static auto scene_letters(const std::vector<uint8_t>&pack,const std::string&text,unsigned size,unsigned face){return spire_test::scene_letters(app.content,pack,text,size,face);}
static std::string scene_text(const char*key){return app.content.world_string("main_menu_ui",key);}
static void scene_replace(std::string&text,const char*key,const char*value){const auto at=text.find(key);SCENE(at!=std::string::npos);text.replace(at,std::strlen(key),value);}
static void profile_capture(const char*folder,const char*name){
    if(!folder)return;const std::string path=std::string(folder)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");SCENE(f);
    std::fprintf(f,"P6\n320 240\n255\n");for(auto p:app.pixels){const uint8_t rgb[]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};SCENE(std::fwrite(rgb,1,3,f)==3);}SCENE(!std::fclose(f));
}
static void compendium_scene(const std::vector<uint8_t>&pack){
    const char*keys[]={"COMPENDIUM_CARD_LIBRARY","COMPENDIUM_RELIC_COLLECTION","COMPENDIUM_POTION_LAB","COMPENDIUM_BESTIARY","STATISTICS","RUN_HISTORY"};
    const int bases[]={I7_MenuCardPanel,I7_MenuRelicPanel,I7_MenuPotionPanel,I7_MenuBestiaryPanel,I7_MenuStatsPanel,I7_MenuHistoryPanel};
    const int icons[]={I7_MenuCardIcon,I7_MenuRelicIcon,I7_MenuPotionIcon,I7_MenuBestiaryIcon,I7_MenuStatsIcon,I7_MenuHistoryIcon};
    for(unsigned index=0;index<6;++index)for(unsigned state=0;state<6;++state){
        const bool bottom=index>=4,focused=state>0;app.pointer.visible=true;app.pointer.x=focused?22:319;app.pointer.y=focused?42:0;app.pointer.held=false;
        app.compendium_motion[index]={};app.map_clock_ms=2000;
        const auto paint=[&](bool visible){for(auto&pixel:app.pixels)pixel=scene_rgb(21,40,65);app.hit_count=0;Canvas canvas{app};if(!visible)canvas.text_visible_characters=0;
            compendium_button(canvas,index,20,40,bottom?53:70,bottom?38:93,bases[index],icons[index],keys[index],UiAction::OpenCompendium,bottom);};
        paint(true);app.map_clock_ms+=50;paint(true);
        if(state>=2){app.pointer.held=true;app.pointer.captured.kind=HitKind::UiAction;app.pointer.captured.value=int(UiAction::OpenCompendium);paint(true);app.map_clock_ms+=state==2?100:200;paint(true);}
        if(state>=4){app.pointer.held=false;app.pointer.x=319;paint(true);app.map_clock_ms+=state==4?150:300;paint(true);}
        const std::vector<uint16_t>actual(app.pixels,app.pixels+320*240);
        paint(false);std::vector<uint16_t>expected(app.pixels,app.pixels+320*240);
        const unsigned base=state==0||state==5?1000:state==1?(bottom?1050:1020):state==2?(bottom?963:985):state==3?(bottom?950:980):bottom?993:997;
        const unsigned brightness=state<2||state==5?255:state==2||state==4?223:191;
        const int scale=int((base*4096+500)/1000),pivot_x=bottom?46:54,pivot_y=bottom?58:86;
        std::string key=std::string(keys[index])+".title";
        scene_label(expected,scene_letters(pack,scene_text(key.c_str()),5,app.content.english()?5:0),bottom?23:24,bottom?66:47,bottom?47:60,10,5,
            scene_rgb(239,200,81),32,2,scene_rgb(51,49,39),1,true,false,255,pivot_x,pivot_y,scale,false,1,1,255,brightness);
        if(!bottom){key=std::string(keys[index])+".description";
            scene_label(expected,scene_letters(pack,scene_text(key.c_str()),5,0),28,96,53,29,5,scene_rgb(255,246,226),64,0,0,0,true,true,255,pivot_x,pivot_y,scale,false,1,0,255,brightness);}
        unsigned ink=0;for(unsigned i=0;i<actual.size();++i){if(actual[i]!=expected[i])std::fprintf(stderr,"compendium=%u state=%u pixel=%u,%u actual=%u expected=%u\n",index,state,i%320,i/320,actual[i],expected[i]);SCENE(actual[i]==expected[i]);ink+=actual[i]!=app.pixels[i];}SCENE(ink>0);
    }
    // Interruption retains both the parent shade and current shader value;
    // it does not jump to a baked press material or restart from white.
    UiMenuTransition transition{};auto visual=transition.observe_compendium(1,1000,false);
    SCENE(visual.scale==1000&&visual.shade==255&&visual.focus==0);
    visual=transition.observe_compendium(2,1025,false);SCENE(visual.scale==1010&&visual.shade==255&&visual.focus==128);
    visual=transition.observe_compendium(1,1125,false);SCENE(visual.scale==984&&visual.shade==223&&visual.focus==128);
    visual=transition.observe_compendium(1,1175,false);SCENE(visual.scale==1020&&visual.shade==223&&visual.focus==255);
    transition.observe_compendium(0,1175,false);visual=transition.observe_compendium(0,1475,false);SCENE(visual.scale==1000&&visual.shade==255&&visual.focus==0);
}
static void title_source_scene(const std::vector<uint8_t>&pack,const char*captures){
    SCENE(title_scene::MenuCenter==111&&title_scene::MenuHitLeft==24&&title_scene::MenuHitWidth==174&&title_scene::MenuFont==10);
    const int fresh_y[]={126,146,166,186},saved_y[]={106,126,146,166,186};
    for(int i=0;i<4;++i)SCENE(title_scene::menu_y(4,i)==fresh_y[i]);for(int i=0;i<5;++i)SCENE(title_scene::menu_y(5,i)==saved_y[i]);
    SCENE(title_scene::ContinueX==184&&title_scene::ContinueWidth==130&&title_scene::continue_y(5)==116);
    SCENE(title_scene::continue_height(false)==82&&title_scene::continue_height(true)==97);
    title_menu_motion::Label motion{};auto value=motion.observe(true,false,1000);SCENE(value.scale==4096&&value.green==246);
    value=motion.sample(1050);SCENE(value.scale==4301&&value.red==239&&value.green==200&&value.blue==81&&value.alpha==255);
    motion.observe(true,true,1050);value=motion.sample(1250);SCENE(value.scale==3891&&value.green==200&&value.alpha==255);
    value=motion.sample(1450);SCENE(value.scale==3891&&value.red==255&&value.green==255&&value.alpha==128);
    motion.observe(true,false,1450);value=motion.sample(1650);SCENE(value.scale==4301&&value.alpha==128);
    value=motion.sample(1850);SCENE(value.scale==4301&&value.green==246&&value.blue==226&&value.alpha==255);
    motion.observe(false,false,1850);SCENE(motion.sample(2350).scale==4096);
    title_menu_motion::Continue info{};SCENE(info.observe(true,1000).alpha==0);SCENE(info.sample(1100).alpha==127);
    SCENE(info.sample(1200).alpha==255&&info.sample(1200).y==-5120);info.observe(false,1200);SCENE(info.sample(1300).alpha==128&&info.sample(1400).alpha==0);
    if(pack.empty())return;
    app.title=true;app.profiles_open=false;app.menu=2;
    const char*keys[2][5]={{"SINGLE_PLAYER","SETTINGS","COMPENDIUM","QUIT",nullptr},{"CONTINUE","ABANDON_RUN","SETTINGS","COMPENDIUM","QUIT"}};
    for(unsigned saved=0;saved<2;++saved)for(unsigned state=0;state<5;++state){
        const unsigned count=saved?5:4;app.has_save=saved;app.title_presentation={};app.map_clock_ms=3000;app.pointer={};app.pointer.visible=state==0;
        app.pointer.x=319;app.pointer.y=0;app.menu=2;
        const auto paint=[&](bool visible){for(auto&pixel:app.pixels)pixel=scene_rgb(21,40,65);app.hit_count=0;Canvas c{app};if(!visible)c.text_visible_characters=0;title_menu_labels(c);};
        paint(true);if(state){app.map_clock_ms+=50;paint(true);}
        if(state>=2){app.pointer.visible=true;app.pointer.x=125;app.pointer.y=saved?176:181;app.pointer.held=true;
            app.pointer.captured={106,int16_t(saved?171:176),38,10,HitKind::Menu,2,true};app.pointer.context=app.input_context();paint(true);
            app.map_clock_ms+=state==2?200:400;paint(true);}
        if(state==4){app.pointer.held=false;paint(true);app.map_clock_ms+=400;paint(true);}
        const std::vector<uint16_t>actual(app.pixels,app.pixels+320*240);std::vector<uint16_t>expected(320*240,scene_rgb(21,40,65));
        for(unsigned i=0;i<count;++i){auto letters=scene_letters(pack,scene_text(keys[saved][i]),6,0);
            if(app.content.english())for(unsigned n=0;n+1<letters.size();++n)letters[n].advance+=((int(n+1)*8+10)/21)-((int(n)*8+10)/21);
            const int width=int(scene_width(letters)),center_y=((699+(450-int(count)*50)/2+int(i)*50+25)*4+10)/21;
            const bool selected=state&&i==2;const int scale=!selected?4096:state==1||state==4?4301:3891;
            const unsigned alpha=selected&&state==3?128:255;
            const uint16_t color=!selected||state==4?scene_rgb(255,246,226):state==3?scene_rgb(255,255,255):scene_rgb(239,200,81);
            scene_label(expected,letters,125-width/2,center_y-4,width,8,6,color,0,3,0,0,false,false,128,125,center_y,scale,false,1,0,alpha);
        }
        // The authored highlight siblings paint after the Label. Preserve
        // that order when isolating the independently rasterized text; a
        // highlight-under-text baseline changes their overlapping antialias.
        Canvas highlight{app};highlight.pixels=expected.data();highlight.text_visible_characters=0;title_menu_labels(highlight);
        for(unsigned i=0;i<actual.size();++i){if(actual[i]!=expected[i])std::fprintf(stderr,"title saved=%u state=%u pixel=%u,%u actual=%u expected=%u\n",saved,state,i%320,i/320,actual[i],expected[i]);SCENE(actual[i]==expected[i]);}
    }
    // Source coordinates include the inherited half-anchor and the settled
    // -20px show offset: panel(153,154,85,38/44), not an unanchored145px X.
    for(unsigned ascension:{0u,3u})for(unsigned saved_at:{0u,1792931400u,1790771400u}){
        app.has_save=true;app.saved_world=true;app.saved_current_region=unsigned(reference::ActKind::Overgrowth);app.saved_character=Character::Silent;
        app.saved_at=saved_at;
        app.saved_floor=42;app.saved_hp=73;app.saved_max_hp=80;app.saved_gold=109;app.saved_ascension=uint8_t(ascension);app.menu=0;
        app.pointer={};app.title_presentation={};app.title_presentation.bind(5);app.map_clock_ms=5000;
        const auto paint=[&](bool visible){for(auto&pixel:app.pixels)pixel=scene_rgb(21,40,65);Canvas c{app};if(!visible)c.text_visible_characters=0;continue_run_info(c);};
        paint(true);app.map_clock_ms+=200;paint(true);const std::vector<uint16_t>actual(app.pixels,app.pixels+320*240);paint(false);std::vector<uint16_t>expected(app.pixels,app.pixels+320*240);
        const bool longest=saved_at==1790771400u;
        std::string date=scene_text("CONTINUE_RUN_INFO.saved");scene_replace(date,"{LastSavedTime}",!saved_at?"—":app.content.english()?(longest?"Wednesday Sep 30, 2026 12:30PM":"Sunday Oct 25, 2026 12:30PM"):(longest?"2026 9月 30， 星期三 下午12:30":"2026 10月 25， 星期日 下午12:30"));
        // Source DateLabel is autowrap=Off. A translated Godot measurement
        // with the source fonts retains24px for unknown/Sunday/Wednesday;
        // its two actual lines begin at0 and30.5/36 source pixels. Keep this
        // oracle separate from fitted_text's automatic line breaker.
        const auto newline=date.find('\n');SCENE(newline!=std::string::npos&&date.find('\n',newline+1)==std::string::npos);
        for(unsigned line=0;line<2;++line){const auto text=line?date.substr(newline+1):date.substr(0,newline);const auto letters=scene_letters(pack,text,5,0);
            SCENE(!letters.empty());SCENE(157+scene_width(letters)<320);
            scene_label(expected,letters,157,157+int(line)*(app.content.english()?6:7),76,17,5,scene_rgb(255,246,226),64,0,0,0,false,false,255,0,0,4096,false,1,1,255,255,true);}
        const std::string progress=std::string(app.content.world_title("acts","OVERGROWTH"))+" [blue]- "+scene_text("CONTINUE_RUN_INFO.floor")+" 42[/blue]";
        scene_label(expected,scene_letters(pack,progress,5,0),165,174,68,7,5,scene_rgb(255,246,226),64,0,0,0,false,false,255,0,0,4096,false,1,1);
        if(ascension)scene_label(expected,scene_letters(pack,scene_text("CONTINUE_RUN_INFO.ascension")+" 3",5,0),157,181,76,7,5,scene_rgb(255,246,226),64,0,0,0,false,false,255,0,0,4096,false,1,1);
        scene_label(expected,scene_letters(pack,"[red]73/80[/red]",5,0),165,ascension?188:181,27,7,5,scene_rgb(255,246,226),64,0,0,0,false,false,255,0,0,4096,false,1,1);
        scene_label(expected,scene_letters(pack,"[gold]109[/gold]",5,0),201,ascension?188:181,19,7,5,scene_rgb(255,246,226),64,0,0,0,false,false,255,0,0,4096,false,1,1);
        for(unsigned i=0;i<actual.size();++i){if(actual[i]!=expected[i])std::fprintf(stderr,"Continue asc=%u saved_at=%u pixel=%u,%u actual=%u expected=%u\n",ascension,saved_at,i%320,i/320,actual[i],expected[i]);SCENE(actual[i]==expected[i]);}
        Canvas c{app};title_screen(c);app.map_clock_ms+=250;title_screen(c);profile_capture(captures,longest?(ascension?"title-source-continue-long-ascension":"title-source-continue-long"):saved_at?(ascension?"title-source-continue-saved-ascension":"title-source-continue-saved"):(ascension?"title-source-continue-ascension":"title-source-continue"));
    }
    app.saved_at=0;
}
static void profile_scene(const std::vector<uint8_t>&pack,const char*captures){
    UiProfileMotion menu_motion{};SCENE(menu_motion.observe(true,100,true)==0);SCENE(menu_motion.value(125,true)>2048&&menu_motion.value(150,true)==4096);
    menu_motion.observe(false,150,true);SCENE(menu_motion.value(450,true)==0);
    UiProfileMotion panel_motion{};panel_motion.observe(true,100);SCENE(panel_motion.value(125)==2048&&panel_motion.value(150)==4096);
    panel_motion.observe(false,150);SCENE(panel_motion.value(650)==0);
    app.title=true;app.profiles_open=true;app.profile_pending=false;app.frontend_confirm=0;app.pointer.visible=true;app.pointer.x=319;app.pointer.y=239;
    app.profile_summaries[0]={3723,1792838400,true};app.profile_summaries[1]={};app.profile_summaries[2]={0,0,true};
    for(auto&motion:app.profile_motion)motion={};for(auto&motion:app.profile_labels)motion={};app.map_clock_ms=1000;
    app.hit_count=0;Canvas menu{app};profile_title_button(menu);bool found=false;
    for(unsigned i=0;i<unsigned(app.hit_count);++i){const auto&hit=app.hits[i];if(hit.kind==HitKind::UiAction&&hit.value==int(UiAction::OpenProfiles)){SCENE(hit.x==3&&hit.y==3&&hit.w==88&&hit.h==23);found=true;}}SCENE(found);
    Canvas page{app};profiles_screen(page);unsigned deletes=0;
    for(unsigned i=0;i<unsigned(app.hit_count);++i){const auto&hit=app.hits[i];if(hit.kind==HitKind::UiAction&&hit.value>=int(UiAction::DeleteProfile1)&&hit.value<=int(UiAction::DeleteProfile3)){
        SCENE(hit.w==15&&hit.h==15&&hit.y==198);SCENE(hit.value!=int(UiAction::DeleteProfile2));++deletes;}}SCENE(deletes==2);
    if(pack.empty()){title_source_scene(pack,captures);return;}
    new_run(app.game,0x1735,false,Character::Ironclad);SCENE(save(app.game,before));
    const auto compare=[&](const std::vector<uint16_t>&actual,const std::vector<uint16_t>&expected){unsigned visible=0;
        for(unsigned i=0;i<actual.size();++i){if(actual[i]!=expected[i]){std::fprintf(stderr,"profile pixel x=%u y=%u actual=%u expected=%u\n",i%320,i/320,actual[i],expected[i]);SCENE(false);}visible+=actual[i]!=app.pixels[i];}SCENE(visible>0);};
    for(unsigned focused=0;focused<2;++focused){
        app.pointer.visible=!focused;app.profile_selection=0;
        if(focused){profiles_screen(page);app.map_clock_ms+=50;}
        Canvas visible{app};profiles_screen(visible);const std::vector<uint16_t>actual(app.pixels,app.pixels+ScreenW*ScreenH);
        Canvas hidden{app};hidden.text_visible_characters=0;profiles_screen(hidden);std::vector<uint16_t>expected(app.pixels,app.pixels+ScreenW*ScreenH);
        scene_label(expected,scene_letters(pack,scene_text("PROFILE_SCREEN.BUTTON.chooseProfileMessage"),7,0),60,24,201,18,7,scene_rgb(239,200,81),128);
        for(unsigned slot=0;slot<3;++slot){const int x=30+88*int(slot),y=67,scale=focused&&!slot?4219:4096;
            std::string title=scene_text("PROFILE_SCREEN.BUTTON.title");const char*ids[]={"1","2","3"};scene_replace(title,"{Id}",ids[slot]);
            scene_label(expected,scene_letters(pack,title,6,app.content.english()?5:0),x+20,y+9,47,8,6,scene_rgb(239,200,81),64,2,scene_rgb(51,47,7),2,true,false,255,x+42,y+53,scale);
            std::string body=scene_text(slot==1?"PROFILE_SCREEN.BUTTON.empty":"PROFILE_SCREEN.BUTTON.description");
            if(slot!=1){scene_replace(body,"{Playtime}",slot?"—":"1:02:03");scene_replace(body,"{LastUpdatedTime}",slot?"—":app.content.english()?"October 24, 2026 10:40 AM":"2026 十月 24， 上午 10:40");}
            scene_label(expected,scene_letters(pack,body,5,0),x+10,y+29,65,69,5,scene_rgb(255,246,226),96,0,0,0,true,true,255,x+42,y+53,scale,true);
        }
        compare(actual,expected);profiles_screen(visible);profile_capture(captures,focused?"profiles-source-hover":"profiles-source-layout");
    }
    app.pointer.visible=true;app.pointer.x=319;app.pointer.y=239;app.profile_motion[6]={};
    for(auto&pixel:app.pixels)pixel=scene_rgb(21,40,65);Canvas visible{app};profile_title_button(visible);
    const std::vector<uint16_t>actual(app.pixels,app.pixels+ScreenW*ScreenH);
    for(auto&pixel:app.pixels)pixel=scene_rgb(21,40,65);Canvas hidden{app};hidden.text_visible_characters=0;profile_title_button(hidden);
    std::vector<uint16_t>expected(app.pixels,app.pixels+ScreenW*ScreenH);std::string title=scene_text("OPEN_PROFILE_SCREEN.title");scene_replace(title,"{Id}","1");
    scene_label(expected,scene_letters(pack,title,5,app.content.english()?5:0),16,4,48,7,5,scene_rgb(239,200,81),0,2,0,0,false,false,64);
    scene_label(expected,scene_letters(pack,scene_text("OPEN_PROFILE_SCREEN.description"),4,0),16,10,55,6,4,scene_rgb(135,206,235),64,0,0,0,false);
    compare(actual,expected);compendium_scene(pack);title_source_scene(pack,captures);SCENE(save(app.game,after)&&identical(before,after));
}
int main(int argc,char**argv){
    std::vector<uint8_t> pack;
    if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);pack.resize(spire_test::pack_size(f));CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());std::fclose(f);CHECK(app.init(pack.data(),pack.size(),{}));}
    char path[64]{};CHECK(profile_scoped_path(path,sizeof path,"nSpire2.progress.tns",1)&&!std::strcmp(path,"nSpire2.progress.tns"));
    CHECK(profile_scoped_path(path,sizeof path,"nSpire2.progress.tns",3)&&!std::strcmp(path,"nSpire2.progress.p3.tns"));
    CHECK(profile_scoped_path(path,sizeof path,"save.bin",2)&&!std::strcmp(path,"save.bin.p2"));
    CHECK(!profile_scoped_path(path,4,"save.bin",2));CHECK(!profile_scoped_path(path,sizeof path,"save.bin",4));
    uint8_t selector[UiProfileSelectorBytes];for(unsigned id=1;id<=3;++id){encode_profile_selector(id,selector);CHECK(decode_profile_selector(selector,sizeof selector)==id);}
    selector[6]=1;CHECK(decode_profile_selector(selector,sizeof selector)==1);CHECK(decode_profile_selector(selector,7)==1);
    app.callbacks.save=write_run;app.callbacks.load=read_run;app.callbacks.clear_save=clear_run;
    app.callbacks.load_preferences=read_preferences;app.callbacks.save_preferences=write_preferences;
    app.fast_mode=true;app.phobia_mode=true;app.long_press=true;app.screen_shake=0;app.bestiary_actions_preferred=false;app.persist_preferences();
    app.callbacks.load_progress=read_progress;app.callbacks.save_progress=write_progress;app.callbacks.wall_clock=wall;
    app.callbacks.current_profile=current;app.callbacks.profile_info=info;app.callbacks.request_profile=request;
    new_run(app.game,0x7231,false,Character::Ironclad);CHECK(world_begin_run(app.game,reference::ActKind::Overgrowth,0));
    app.title=false;app.begin_progress();app.advance(250);CHECK(app.save_current_run());
    before=runs[0];const auto history_one=histories[0];app.return_to_title();CHECK(app.has_save);
    app.ui_action(UiAction::OpenProfiles);CHECK(app.profiles_open&&app.current_profile()==1);
    CHECK(app.profile_summaries[0].exists&&!app.profile_summaries[1].exists&&!app.profile_summaries[2].exists);
    app.input(Key::Down);CHECK(app.profile_selection==3);app.input(Key::Right);CHECK(app.profile_selection==3);
    app.input(Key::Left);CHECK(app.profile_selection==3);app.input(Key::Up);CHECK(app.profile_selection==0);
    app.ui_action(UiAction::SelectProfile2);CHECK(app.profile_pending&&requested==2&&active==1);
    CHECK(save(app.game,after)&&identical(before,after)); // Queuing never adopts another live model.
    service();CHECK(active==2&&!app.profiles_open&&!app.has_save&&!app.run_save_linked);
    CHECK(!app.fast_mode&&!app.phobia_mode&&!app.long_press&&app.screen_shake==2&&app.bestiary_actions_preferred);
    app.show_card_indices=true;app.keyboard_only=true;app.persist_preferences();
    CHECK(app.progress.count==0&&app.progress.current.serial==0&&app.progress.next_serial==1);
    CHECK(identical(runs[0],before)&&histories[0]==history_one);
    new_run(app.game,0x7322,false,Character::Silent);CHECK(world_begin_run(app.game,reference::ActKind::Overgrowth,0));
    app.title=false;app.begin_progress();CHECK(app.save_current_run());app.return_to_title();
    app.ui_action(UiAction::OpenProfiles);app.ui_action(UiAction::SelectProfile1);service();
    CHECK(active==1&&app.has_save);CHECK(app.fast_mode&&app.phobia_mode&&app.long_press&&app.screen_shake==0&&!app.bestiary_actions_preferred);
    CHECK(!app.show_card_indices&&!app.keyboard_only);CHECK(app.load_saved_run()&&app.game.character==Character::Ironclad);
    CHECK(save(app.game,after)&&identical(before,after));app.return_to_title();
    app.ui_action(UiAction::OpenProfiles);app.input(Key::Down);CHECK(app.profile_selection==3);
    app.input(Key::Left);CHECK(app.profile_selection==4);app.input(Key::Right);CHECK(app.profile_selection==3);app.input(Key::Up);
    app.ui_action(UiAction::OpenProfiles);app.ui_action(UiAction::DeleteProfile2);
    CHECK(app.frontend_confirm==5&&app.profile_target==2&&!requested);
    if(app.content.version()>=Ui6ContentVersion){app.render();app.advance(150);app.advance(150);app.render();unsigned found=0;
        for(int i=0;i<app.hit_count;++i){const auto&h=app.hits[i];if(h.kind!=HitKind::UiAction)continue;
            const bool yes=h.value==int(UiAction::FrontendAccept),no=h.value==int(UiAction::FrontendCancel);if(!yes&&!no)continue;
            CHECK(h.x==((yes?921:528)*4+10)/21&&h.y==(778*4+10)/21&&h.w==(180*4+10)/21&&h.h==(72*4+10)/21);found|=yes?2:1;}
        CHECK(found==3);profile_capture(argc>2?argv[2]:nullptr,"profiles-source-delete-confirm");
    }
    app.input(Key::Back);CHECK(!app.frontend_confirm&&has_run[1]&&!histories[1].empty());
    app.ui_action(UiAction::DeleteProfile2);app.frontend_choice=1;app.input(Key::Confirm);
    CHECK(requested==2&&erase_requested&&app.profile_pending);service();
    CHECK(active==1&&app.profiles_open&&!has_run[1]&&histories[1].empty());
    CHECK(!preferences[1].show_card_indices&&!preferences[1].keyboard_only&&app.fast_mode);
    CHECK(identical(runs[0],before)&&has_run[0]&&app.has_save);
    storage_ok=false;app.ui_action(UiAction::SelectProfile3);CHECK(active==1&&!requested&&!app.profile_pending&&app.profiles_open);
    storage_ok=true;app.ui_action(UiAction::SelectProfile3);CHECK(app.profile_pending);service(false);
    CHECK(active==1&&app.profiles_open&&app.has_save&&identical(runs[0],before));
    CHECK(app.fast_mode&&app.phobia_mode&&app.long_press&&app.screen_shake==0&&!app.bestiary_actions_preferred);
    app.ui_action(UiAction::DeleteProfile1);app.frontend_choice=1;app.input(Key::Confirm);service();
    CHECK(active==1&&app.profiles_open&&!app.has_save&&!has_run[0]&&histories[0].empty());
    CHECK(app.progress.count==0&&!app.progress.current.serial&&app.progress.next_serial==1);
    CHECK(!app.fast_mode&&!app.phobia_mode&&!app.long_press&&app.screen_shake==2&&app.bestiary_actions_preferred);
    app.title=false;app.profiles_open=false;app.open_profiles();CHECK(!app.profiles_open); // In-run switching is unavailable.
    profile_scene(pack,argc>2?argv[2]:nullptr);
    std::puts("profiles: three independent runs/progress, confirmation, failed switch and active deletion passed");
}
