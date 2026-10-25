#include "../src/app.cpp"
#include "content_fixture.hpp"
#include "world_run.hpp"
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
struct SceneLetter {unsigned cp,width,height,advance,color;std::vector<uint8_t>alpha;};
static std::vector<SceneLetter> scene_letters(const std::vector<uint8_t>&pack,const std::string&text,unsigned size,unsigned face){
    const auto u32=[&](unsigned at){return unsigned(pack[at])|unsigned(pack[at+1])<<8|unsigned(pack[at+2])<<16|unsigned(pack[at+3])<<24;};
    const unsigned glyphs=u32(20),base=40+u32(16)*16;std::vector<SceneLetter>letters;
    RichReader reader(text.c_str());RichChar ch{};
    while(reader.next(ch)){
        if(ch.codepoint=='\n'){letters.push_back({ch.codepoint,0,0,0,ch.style.color,{}});continue;}
        const unsigned selected=face|(ch.style.bold?5u:0u),band=size|((selected&3)<<6)|((selected&4)<<3);
        const uint64_t key=uint64_t(band)<<24|ch.codepoint;unsigned low=0,high=glyphs;
        while(low<high){const unsigned mid=(low+high)/2,at=base+mid*16;
            const uint64_t value=uint64_t(pack[at+8]|unsigned(pack[at+9])<<8)<<24|u32(at);if(value<key)low=mid+1;else high=mid;}
        SCENE(low<glyphs);const unsigned at=base+low*16;
        if(u32(at)!=ch.codepoint||(pack[at+8]|unsigned(pack[at+9])<<8)!=band)std::fprintf(stderr,"missing profile glyph cp=%u size=%u face=%u\n",ch.codepoint,size,selected);
        SCENE(u32(at)==ch.codepoint&&(pack[at+8]|unsigned(pack[at+9])<<8)==band);
        const auto glyph=app.content.glyph(ch.codepoint,int(size),selected);SCENE(glyph.pixels&&glyph.advance);
        SceneLetter letter{ch.codepoint,glyph.w,glyph.h,glyph.advance,ch.style.color,std::vector<uint8_t>(unsigned(glyph.w)*glyph.h)};
        if(glyph.encoded_size){unsigned pos=0;for(unsigned i=0;i<glyph.encoded_size;i+=2){SCENE(i+1<glyph.encoded_size&&pos+glyph.pixels[i]<=letter.alpha.size());
            for(unsigned n=0;n<glyph.pixels[i];++n)letter.alpha[pos++]=glyph.pixels[i+1];}SCENE(pos==letter.alpha.size());}
        else std::memcpy(letter.alpha.data(),glyph.pixels,letter.alpha.size());letters.push_back(letter);
    }return letters;
}
static unsigned scene_width(const std::vector<SceneLetter>&letters){unsigned n=0;for(const auto&letter:letters)n+=letter.advance;return n;}
static uint16_t scene_rgb(unsigned r,unsigned g,unsigned b){return uint16_t((r>>3)<<11|(g>>2)<<5|(b>>3));}
static void scene_pixel(std::vector<uint16_t>&frame,int x,int y,uint16_t color,unsigned alpha){
    if(x<0||x>=320||y<0||y>=240)return;const uint16_t old=frame[unsigned(y*320+x)];
    const auto mix=[&](unsigned a,unsigned b){return (a*(255-alpha)+b*alpha+127)/255;};
    frame[unsigned(y*320+x)]=uint16_t(mix(old>>11,color>>11)<<11|mix((old>>5)&63,(color>>5)&63)<<5|mix(old&31,color&31));
}
static void scene_label(std::vector<uint16_t>&frame,const std::vector<SceneLetter>&letters,int x,int y,int width,int height,
                        unsigned size,uint16_t color,unsigned shadow,int outline=0,uint16_t outline_color=0,int shadow_outline=0,
                        bool center=true,bool wrap=false,unsigned outline_alpha=255,int pivot_x=0,int pivot_y=0,int scale=4096,bool clip=false){
    std::vector<std::vector<SceneLetter>>lines(1);unsigned line_width=0;
    for(unsigned i=0;i<letters.size();++i){const auto&letter=letters[i];
        if(letter.cp=='\n'){lines.emplace_back();line_width=0;continue;}
        const bool space=letter.cp==' '||letter.cp=='\t';if(wrap&&space&&(!line_width||line_width+letter.width>unsigned(width)))continue;
        if(wrap&&line_width){unsigned word_width=0;if((i==0||letters[i-1].cp==' '||letters[i-1].cp=='\n')&&letter.cp>32&&letter.cp<0x300){
                unsigned cursor=0;for(unsigned n=i;n<letters.size()&&letters[n].cp>32&&letters[n].cp<0x300;++n){word_width=cursor+std::max(letters[n].width,letters[n].advance);cursor+=letters[n].advance;}}
            if(line_width+std::max(letter.width,letter.advance)>unsigned(width)||(word_width&&word_width<=unsigned(width)&&line_width+word_width>unsigned(width))){lines.emplace_back();line_width=0;}}
        if(wrap&&space&&!line_width)continue;lines.back().push_back(letter);line_width+=letter.advance;
    }
    const int top=y+(height-(int(lines.size())*int(size+1)+1))/2;
    const auto project=[&](int value,int pivot){const int delta=(value-pivot)*scale;return pivot+(delta+(delta<0?-2048:2048))/4096;};
    const auto pass=[&](int dx,int dy,unsigned opacity,uint16_t ink,bool decoration){
        for(unsigned row=0;row<lines.size();++row){int at=x+(center?(width-int(scene_width(lines[row])))/2:0);
            for(const auto&letter:lines[row]){const uint16_t rich=letter.color==3?scene_rgb(135,206,235):letter.color==1?scene_rgb(239,200,81):color;
                for(unsigned yy=0;yy<letter.height;++yy)for(unsigned xx=0;xx<letter.width;++xx){const unsigned alpha=(letter.alpha[yy*letter.width+xx]*opacity+127)/255;
                    const int px=at+int(xx)+dx,py=top+int(row*(size+1))+int(yy)+dy;
                    for(int sy=project(py,pivot_y);sy<project(py+1,pivot_y);++sy)for(int sx=project(px,pivot_x);sx<project(px+1,pivot_x);++sx){
                        if(clip&&(sx<project(x,pivot_x)||sx>=project(x+width,pivot_x)||sy<project(y,pivot_y)||sy>=project(y+height,pivot_y)))continue;
                        scene_pixel(frame,sx,sy,decoration?ink:rich,alpha);}}
                at+=letter.advance;}}
    };
    if(shadow)for(int yy=-shadow_outline;yy<=shadow_outline;++yy)for(int xx=-shadow_outline;xx<=shadow_outline;++xx)
        if(xx*xx+yy*yy<=shadow_outline*shadow_outline)pass(xx+1,yy,shadow,0,true);
    for(int yy=-outline;yy<=outline;++yy)for(int xx=-outline;xx<=outline;++xx)
        if((xx||yy)&&xx*xx+yy*yy<=outline*outline)pass(xx,yy,outline_alpha,outline_color,true);
    pass(0,0,255,color,false);
}
static std::string scene_text(const char*key){return app.content.world_string("main_menu_ui",key);}
static void scene_replace(std::string&text,const char*key,const char*value){const auto at=text.find(key);SCENE(at!=std::string::npos);text.replace(at,std::strlen(key),value);}
static void profile_capture(const char*folder,const char*name){
    if(!folder)return;const std::string path=std::string(folder)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");SCENE(f);
    std::fprintf(f,"P6\n320 240\n255\n");for(auto p:app.pixels){const uint8_t rgb[]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};SCENE(std::fwrite(rgb,1,3,f)==3);}SCENE(!std::fclose(f));
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
    for(unsigned i=0;i<unsigned(app.hit_count);++i){const auto&hit=app.hits[i];if(hit.kind==HitKind::UiAction&&hit.value==int(UiAction::OpenProfiles)){SCENE(hit.x==3&&hit.y==3&&hit.w==33&&hit.h==12);found=true;}}SCENE(found);
    Canvas page{app};profiles_screen(page);unsigned deletes=0;
    for(unsigned i=0;i<unsigned(app.hit_count);++i){const auto&hit=app.hits[i];if(hit.kind==HitKind::UiAction&&hit.value>=int(UiAction::DeleteProfile1)&&hit.value<=int(UiAction::DeleteProfile3)){
        SCENE(hit.w==15&&hit.h==15&&hit.y==192);SCENE(hit.value!=int(UiAction::DeleteProfile2));++deletes;}}SCENE(deletes==2);
    if(pack.empty())return;
    new_run(app.game,0x1735,false,Character::Ironclad);SCENE(save(app.game,before));
    const auto compare=[&](const std::vector<uint16_t>&actual,const std::vector<uint16_t>&expected){unsigned visible=0;
        for(unsigned i=0;i<actual.size();++i){if(actual[i]!=expected[i]){std::fprintf(stderr,"profile pixel x=%u y=%u actual=%u expected=%u\n",i%320,i/320,actual[i],expected[i]);SCENE(false);}visible+=actual[i]!=app.pixels[i];}SCENE(visible>0);};
    for(unsigned focused=0;focused<2;++focused){
        app.pointer.visible=!focused;app.profile_selection=0;
        if(focused){profiles_screen(page);app.map_clock_ms+=50;}
        Canvas visible{app};profiles_screen(visible);const std::vector<uint16_t>actual(app.pixels,app.pixels+ScreenW*ScreenH);
        Canvas hidden{app};hidden.text_visible_characters=0;profiles_screen(hidden);std::vector<uint16_t>expected(app.pixels,app.pixels+ScreenW*ScreenH);
        scene_label(expected,scene_letters(pack,scene_text("PROFILE_SCREEN.BUTTON.chooseProfileMessage"),7,0),60,24,201,18,7,scene_rgb(239,200,81),128);
        for(unsigned slot=0;slot<3;++slot){const int x=30+88*int(slot)+(slot==2?1:0),y=67,scale=focused&&!slot?4219:4096;
            std::string title=scene_text("PROFILE_SCREEN.BUTTON.title");const char*ids[]={"1","2","3"};scene_replace(title,"{Id}",ids[slot]);
            scene_label(expected,scene_letters(pack,title,6,5),x+20,y+9,47,8,6,scene_rgb(239,200,81),64,2,scene_rgb(51,47,7),2,true,false,255,x+42,y+53,scale);
            std::string body=scene_text(slot==1?"PROFILE_SCREEN.BUTTON.empty":"PROFILE_SCREEN.BUTTON.description");
            if(slot!=1){scene_replace(body,"{Playtime}",slot?"—":"1:02:03");scene_replace(body,"{LastUpdatedTime}",slot?"—":app.content.english()?"October 24, 2026 10:40 AM":"2026/10/24 10:40");}
            scene_label(expected,scene_letters(pack,body,5,0),x+10,y+29,65,69,5,scene_rgb(255,246,226),96,0,0,0,true,true,255,x+42,y+53,scale,true);
        }
        compare(actual,expected);profiles_screen(visible);profile_capture(captures,focused?"profiles-source-hover":"profiles-source-layout");
    }
    app.pointer.visible=true;app.pointer.x=319;app.pointer.y=239;app.profile_motion[6]={};
    for(auto&pixel:app.pixels)pixel=scene_rgb(21,40,65);Canvas visible{app};profile_title_button(visible);
    const std::vector<uint16_t>actual(app.pixels,app.pixels+ScreenW*ScreenH);
    for(auto&pixel:app.pixels)pixel=scene_rgb(21,40,65);Canvas hidden{app};hidden.text_visible_characters=0;profile_title_button(hidden);
    std::vector<uint16_t>expected(app.pixels,app.pixels+ScreenW*ScreenH);std::string title=scene_text("OPEN_PROFILE_SCREEN.title");scene_replace(title,"{Id}","1");
    scene_label(expected,scene_letters(pack,title,5,5),16,4,48,7,5,scene_rgb(239,200,81),0,2,0,0,false,false,64);
    scene_label(expected,scene_letters(pack,scene_text("OPEN_PROFILE_SCREEN.description"),4,0),16,10,55,6,4,scene_rgb(135,206,235),64,0,0,0,false);
    compare(actual,expected);SCENE(save(app.game,after)&&identical(before,after));
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
    app.fast_mode=true;app.phobia_mode=true;app.screen_shake=0;app.bestiary_actions_preferred=false;app.persist_preferences();
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
    CHECK(!app.fast_mode&&!app.phobia_mode&&app.screen_shake==2&&app.bestiary_actions_preferred);
    app.show_card_indices=true;app.keyboard_only=true;app.persist_preferences();
    CHECK(app.progress.count==0&&app.progress.current.serial==0&&app.progress.next_serial==1);
    CHECK(identical(runs[0],before)&&histories[0]==history_one);
    new_run(app.game,0x7322,false,Character::Silent);CHECK(world_begin_run(app.game,reference::ActKind::Overgrowth,0));
    app.title=false;app.begin_progress();CHECK(app.save_current_run());app.return_to_title();
    app.ui_action(UiAction::OpenProfiles);app.ui_action(UiAction::SelectProfile1);service();
    CHECK(active==1&&app.has_save);CHECK(app.fast_mode&&app.phobia_mode&&app.screen_shake==0&&!app.bestiary_actions_preferred);
    CHECK(!app.show_card_indices&&!app.keyboard_only);CHECK(app.load_saved_run()&&app.game.character==Character::Ironclad);
    CHECK(save(app.game,after)&&identical(before,after));app.return_to_title();
    app.ui_action(UiAction::OpenProfiles);app.input(Key::Down);CHECK(app.profile_selection==3);
    app.input(Key::Left);CHECK(app.profile_selection==4);app.input(Key::Right);CHECK(app.profile_selection==3);app.input(Key::Up);
    app.ui_action(UiAction::OpenProfiles);app.ui_action(UiAction::DeleteProfile2);
    CHECK(app.frontend_confirm==5&&app.profile_target==2&&!requested);
    app.input(Key::Back);CHECK(!app.frontend_confirm&&has_run[1]&&!histories[1].empty());
    app.ui_action(UiAction::DeleteProfile2);app.frontend_choice=1;app.input(Key::Confirm);
    CHECK(requested==2&&erase_requested&&app.profile_pending);service();
    CHECK(active==1&&app.profiles_open&&!has_run[1]&&histories[1].empty());
    CHECK(!preferences[1].show_card_indices&&!preferences[1].keyboard_only&&app.fast_mode);
    CHECK(identical(runs[0],before)&&has_run[0]&&app.has_save);
    storage_ok=false;app.ui_action(UiAction::SelectProfile3);CHECK(active==1&&!requested&&!app.profile_pending&&app.profiles_open);
    storage_ok=true;app.ui_action(UiAction::SelectProfile3);CHECK(app.profile_pending);service(false);
    CHECK(active==1&&app.profiles_open&&app.has_save&&identical(runs[0],before));
    CHECK(app.fast_mode&&app.phobia_mode&&app.screen_shake==0&&!app.bestiary_actions_preferred);
    app.ui_action(UiAction::DeleteProfile1);app.frontend_choice=1;app.input(Key::Confirm);service();
    CHECK(active==1&&app.profiles_open&&!app.has_save&&!has_run[0]&&histories[0].empty());
    CHECK(app.progress.count==0&&!app.progress.current.serial&&app.progress.next_serial==1);
    CHECK(!app.fast_mode&&!app.phobia_mode&&app.screen_shake==2&&app.bestiary_actions_preferred);
    app.title=false;app.profiles_open=false;app.open_profiles();CHECK(!app.profiles_open); // In-run switching is unavailable.
    profile_scene(pack,argc>2?argv[2]:nullptr);
    std::puts("profiles: three independent runs/progress, confirmation, failed switch and active deletion passed");
}
