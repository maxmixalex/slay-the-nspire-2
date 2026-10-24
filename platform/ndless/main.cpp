#include "app.hpp"
#include "resource_file_cursor.hpp"
#include "runtime_performance.hpp"
#include "runtime_scene.hpp"
#include "touchpad.hpp"
#include "loading_screen.hpp"
extern "C" {
void* nd_open(const char*,const char*);
unsigned nd_read(void*,unsigned,unsigned,void*);
unsigned nd_write(const void*,unsigned,unsigned,void*);
int nd_close(void*);
int nd_seek(void*,long,int);
long nd_tell(void*);
int nd_rename(const char*,const char*);
int nd_unlink(const char*);
unsigned nd_hwtype();
unsigned nd_revision();
int nd_lcd_init(int);
void (*nd_blitter(int))(void*);
int nd_touch_read(unsigned,unsigned,void*);
int nd_touch_write(unsigned,unsigned,const void*);
}
static spire::App app;
static spire::SaveData scratch;
static spire::RuntimePerformance performance;
static char pack_path[512],save_path[512],temp_path[512],backup_path[512],performance_path[512],profile_path[512],language_path[512];
static char settings_path[512],settings_temp[512],settings_backup[512];
static uint32_t timer_ticks(){return *(volatile uint32_t*)0x900D0004u;}
static uint32_t entropy_start=0;
static unsigned installed_languages=0,pending_language=0;
static unsigned available_languages(){return installed_languages;}
static bool request_language(unsigned language){
    if(language<1||language>2||!(installed_languages&(1u<<(language-1))))return false;
    pending_language=language;return true;
}
static uint32_t seed_entropy(){
    // Same read-only RTC used by the official Ndless _gettimeofday. Mix the
    // current input-time subsecond counter and pre-init timing; never reset
    // the OS clock or consume/replace a seeded run's simulation RNG stream.
    const uint32_t rtc=*(volatile uint32_t*)0x90090000u,ticks=timer_ticks();
    entropy_start+=0x9e3779b9u;
    uint32_t value=rtc^entropy_start^((ticks<<11)|(ticks>>21));
    value^=value>>16;value*=0x7feb352du;value^=value>>15;value*=0x846ca68bu;value^=value>>16;
    return value?value:0xa341316cu;
}
struct ResourceFile {
    void*file=nullptr;spire::ResourceFileCursor cursor;
    void(*blit)(void*)=nullptr;
    uint32_t drawn=0,phase_start=0;
    spire::ResourceStage phase=spire::ResourceStage::Count;
    spire::BootLanguage language=spire::BootLanguage::Unknown;
    void close(){if(file){nd_close(file);file=nullptr;}cursor={};}
    ~ResourceFile(){close();}
};
static void resource_progress(void*context,spire::ResourceStage stage,uint32_t completed,uint32_t total){
    auto&s=*static_cast<ResourceFile*>(context);const uint32_t now=timer_ticks();
    const bool changed=stage!=s.phase;
    if(changed){
        if(performance.enabled&&s.phase!=spire::ResourceStage::Count)
            performance.startup_phases[(unsigned)s.phase].add(s.phase_start-now);
        s.phase=stage;s.phase_start=now;
    }
    // Startup no longer presents a synthetic animation. Keep only opt-in
    // phase accounting; the first actual UI frame follows the bounded open.
    (void)completed;(void)total;
}
static bool read_resource(void* context,uint32_t offset,void* output,uint32_t count){
    auto&source=*static_cast<ResourceFile*>(context);bool seek=false;
    uint32_t started=performance.enabled?timer_ticks():0;
    bool good=source.cursor.read(source.file,offset,output,count,nd_seek,nd_read,seek);
    if(performance.enabled)performance.read(started-timer_ticks(),count,good,seek);
    return good;
}
static bool sibling(char*out,const char*launcher,const char*filename){
    int slash=-1,n=0;while(launcher&&launcher[n]){if(n>=420)return false;if(launcher[n]=='/')slash=n;n++;}
    int at=0;while(at<=slash){out[at]=launcher[at];at++;}while(*filename&&at<510)out[at++]=*filename++;out[at]=0;return !*filename;
}
static void* open_sibling(const char*launcher,const char*name){return sibling(pack_path,launcher,name)?nd_open(pack_path,"rb"):nullptr;}
static spire::ResourceSource file_source(ResourceFile&file){
    file.cursor={}; // This length probe moves the underlying stream as well.
    if(!file.file||nd_seek(file.file,0,2)!=0)return {};
    const long length=nd_tell(file.file);
    if(length<=0||(unsigned long)length>spire::MaxContentFile)return {};
    return {&file,(uint32_t)length,read_resource,nullptr,resource_progress};
}
static bool load_preferences(spire::UiPreferences& value){
    void*f=nd_open(settings_path,"rb");if(!f)return false;
    uint8_t bytes[spire::UiPreferencesBytes+1];const unsigned n=nd_read(bytes,1,sizeof bytes,f);
    nd_close(f);return spire::decode_ui_preferences(bytes,n,value);
}
static bool save_preferences(const spire::UiPreferences& value){
    uint8_t bytes[spire::UiPreferencesBytes];spire::encode_ui_preferences(value,bytes);
    void*f=nd_open(settings_temp,"wb");if(!f)return false;
    bool ok=nd_write(bytes,1,sizeof bytes,f)==sizeof bytes;ok=(nd_close(f)==0)&&ok;
    if(!ok){nd_unlink(settings_temp);return false;}
    bool old=false;f=nd_open(settings_path,"rb");if(f){old=true;nd_close(f);}
    if(old){nd_unlink(settings_backup);if(nd_rename(settings_path,settings_backup)!=0){nd_unlink(settings_temp);return false;}}
    if(nd_rename(settings_temp,settings_path)!=0){if(old)nd_rename(settings_backup,settings_path);return false;}
    if(old)nd_unlink(settings_backup);return true;
}
static bool save_game(const spire::State&s){
    if(!spire::save(s,scratch))return false;
    void*f=nd_open(temp_path,"wb");if(!f)return false;
    bool good=nd_write(scratch.bytes,1,(unsigned)scratch.size,f)==scratch.size;
    good=(nd_close(f)==0)&&good;if(!good){nd_unlink(temp_path);return false;}
    bool old=false;f=nd_open(save_path,"rb");if(f){old=true;nd_close(f);}
    if(old){nd_unlink(backup_path);if(nd_rename(save_path,backup_path)!=0){nd_unlink(temp_path);return false;}}
    if(nd_rename(temp_path,save_path)!=0){if(old)nd_rename(backup_path,save_path);return false;}
    return true;
}
static bool load_game(spire::State&s){
    void*f=nd_open(save_path,"rb");if(!f)return false;
    unsigned n=nd_read(scratch.bytes,1,sizeof scratch.bytes,f);uint8_t extra=0;bool end=nd_read(&extra,1,1,f)==0;nd_close(f);
    return end&&spire::load(s,scratch.bytes,n);
}
static bool clear_save(){
    // A deliberate abandon also removes the preceding atomic-save backup.
    // Otherwise a later recovery could bring back an abandoned expedition.
    const char*paths[]={backup_path,temp_path,save_path};for(unsigned i=0;i<3;i++){const char*path=paths[i];
        void*f=nd_open(path,"rb");if(!f)continue;if(nd_close(f)!=0||nd_unlink(path)!=0)return false;
    }return true;
}
static bool pressed(unsigned offset,unsigned mask){return (*(volatile uint16_t*)(0x900E0000u+offset)&mask)!=0;}
template<class Ticks,class Held>
static bool exit_input_release(Ticks ticks,Held held){
    // Keep the confirming press in this foreground program until input has
    // stayed released for 100ms, just like the entry gate. Ndless r2022 also
    // checks release after main, but accepts the first clear hardware sample.
    // This bounded exit-only poll uses our existing countdown timer; it does
    // not reconfigure IRQs, sleep on the OS's timer, or alter keyboard state.
    const uint32_t started=ticks();uint32_t sampled=started,quiet_since=started;
    bool first=true,quiet=false;
    for(;;){
        const uint32_t now=ticks();
        if(started-now>=65536u)return false; // At most two foreground seconds.
        if(!first&&sampled-now<328u)continue; // Sample input at most every 10ms.
        first=false;sampled=now;
        if(held())quiet=false;
        else if(!quiet){quiet=true;quiet_since=now;}
        else if(quiet_since-now>=3277u)return true;
    }
}
static bool boot_keys_held(){for(unsigned offset=0x10;offset<0x20;offset+=2)if(pressed(offset,0xffffu))return true;return false;}
static void boot_error(void(*blit)(void*),spire::BootText message,spire::BootLanguage language){
    spire::boot_error_screen(app.pixels,message,language);blit(app.pixels);
    exit_input_release(timer_ticks,boot_keys_held);
    while(!pressed(0x1C,0x80)&&!pressed(0x10,2)&&!pressed(0x1A,0x40)){}
    exit_input_release(timer_ticks,boot_keys_held);
}
static spire::BootLanguage language_preference(){
    void*f=nd_open(language_path,"rb");if(!f)return spire::BootLanguage::Unknown;
    uint8_t bytes[9];const unsigned count=nd_read(bytes,1,sizeof bytes,f);nd_close(f);
    return spire::boot_preference(bytes,count);
}
static void store_language(spire::BootLanguage language){
    void*f=nd_open(language_path,"wb");if(!f)return;
    bool good=nd_write(spire::boot_preference_bytes(language),1,8,f)==8;
    good=(nd_close(f)==0)&&good;if(!good)nd_unlink(language_path);
}
struct Clock {
    uint32_t load, control, last;bool active=false;
    // SP804 timer 2 is dedicated to this foreground runtime. Do not reset
    // the OS's 0x900C timer: restoring its registers cannot restore elapsed time.
    void init(){auto*t=(volatile uint32_t*)0x900D0000u;
        load=t[0];control=t[2];t[2]=0;t[0]=0xffffffffu;t[2]=0x82;last=t[1];active=true;}
    unsigned frame(){uint32_t now=*(volatile uint32_t*)0x900D0004u,dt=last-now;
        if(dt<1092)return 0;last=now;return dt>32768?1000:(dt*1000)>>15;}
    void restore(){if(!active)return;auto*t=(volatile uint32_t*)0x900D0000u;t[2]=0;t[0]=load;t[2]=control;active=false;}
    ~Clock(){restore();}
};
struct Display {
    bool active=false;
    void restore(){if(active){nd_lcd_init(-1);active=false;}}
    ~Display(){restore();}
};
static spire::RuntimeScene performance_scene(){
    return spire::runtime_scene(app);
}
static bool performance_write(void*file,const char*bytes,size_t count){return nd_write(bytes,1,(unsigned)count,file)==count;}
static void performance_report(){
    if(!performance.enabled)return;
    void*f=nd_open(performance_path,"wb");if(!f)return;
    bool good=performance.write(performance_write,f);good=(nd_close(f)==0)&&good;
    if(!good)nd_unlink(performance_path); // Do not leave a truncated result looking valid.
}
struct Touchpad {
    unsigned w,h;
    void init(){
        uint32_t cpsr;asm volatile("mrs %0,cpsr":"=r"(cpsr));asm volatile("msr cpsr_c,%0"::"r"(cpsr|0xC0):"memory");
        uint8_t page=16,b[4]={};bool ok=nd_touch_write(255,255,&page)!=0;
        if(ok)ok=nd_touch_read(4,7,b)!=0;page=4;bool restored=nd_touch_write(255,255,&page)!=0;
        asm volatile("msr cpsr_c,%0"::"r"(cpsr):"memory");
        w=ok&&restored?((unsigned)b[0]<<8)|b[1]:0;h=ok&&restored?((unsigned)b[2]<<8)|b[3]:0;
    }
    spire::TouchSample read(){
        uint8_t b[12]={};
        if(!w||!h||!nd_touch_read(0,10,b))return {};
        return {(uint16_t)(((unsigned)b[2]<<8)|b[3]),
                (uint16_t)(((unsigned)b[4]<<8)|b[5]),(uint16_t)w,(uint16_t)h,
                b[0]!=0,b[10]!=0,true};
    }
};
extern "C" int main(int argc,char**argv){
    if(nd_revision()<2022||nd_hwtype()==0)return 2;
    const char*launcher=argc>0&&argv&&argv[0]?argv[0]:"";
    if(!sibling(save_path,launcher,"nSpire2.sav.tns")||!sibling(temp_path,launcher,"nSpire2.tmp.tns")||!sibling(backup_path,launcher,"nSpire2.bak.tns")||!sibling(performance_path,launcher,"nSpire2.perf.tns")||!sibling(profile_path,launcher,"nSpire2.profile.tns")||!sibling(language_path,launcher,"nSpire2.lang.tns")||!sibling(settings_path,launcher,"nSpire2.settings.tns")||!sibling(settings_temp,launcher,"nSpire2.settings.tmp.tns")||!sibling(settings_backup,launcher,"nSpire2.settings.bak.tns"))return 3;
    // Ndless may wait for all launch keys to be released before entering main,
    // so a held CTRL alone cannot reliably opt in. An owner-created sibling
    // marker enables startup timing without a loader/key timing dependency.
    // Open read-only and close immediately; never create/remove this marker.
    performance.enabled=pressed(0x1E,0x200);
    if(void*marker=nd_open(profile_path,"rb")){performance.enabled=true;nd_close(marker);}
    // With no marker and no CTRL diagnostics stay disabled. One bounded report
    // is written on normal exit, with no per-frame filesystem logging.
    performance.hardware=nd_hwtype();performance.ndless_revision=nd_revision();
    entropy_start=*(volatile uint32_t*)0x90090000u^timer_ticks()^*(volatile uint32_t*)0x900C0004u;
    Clock clock{};clock.init();
    Display display;if(!nd_lcd_init(0))return 6;display.active=true;
    auto blit=nd_blitter(0);if(!blit)return 7;
    for(auto&pixel:app.pixels)pixel=0;blit(app.pixels);
    ResourceFile common{},source{};spire::SplitResourceSource split;
    const auto preferred=language_preference();bool selected_language=false;
    common.file=open_sibling(launcher,"nSpire2.common.tns");
    if(common.file){
        void*english=open_sibling(launcher,"nSpire2.eng.tns");void*chinese=open_sibling(launcher,"nSpire2.zhs.tns");
        if(!english&&!chinese){boot_error(blit,spire::BootText::MissingLanguage,spire::BootLanguage::Unknown);return 4;}
        installed_languages=(english?1u:0u)|(chinese?2u:0u);
        source.language=preferred==spire::BootLanguage::English&&english?spire::BootLanguage::English:
            preferred==spire::BootLanguage::Chinese&&chinese?spire::BootLanguage::Chinese:
            chinese?spire::BootLanguage::Chinese:spire::BootLanguage::English;
        if(source.language==spire::BootLanguage::English){source.file=english;if(chinese)nd_close(chinese);}
        else if(source.language==spire::BootLanguage::Chinese){source.file=chinese;if(english)nd_close(english);}
        else {if(english)nd_close(english);if(chinese)nd_close(chinese);exit_input_release(timer_ticks,boot_keys_held);return 0;}
        selected_language=true;
    }else source.file=open_sibling(launcher,"nSpire2.nsp.tns");
    if(!source.file){boot_error(blit,spire::BootText::MissingResources,spire::BootLanguage::Unknown);return 4;}
    const uint32_t startup_start=performance.enabled?timer_ticks():0;
    source.blit=blit;auto resource=file_source(source);bool valid=resource.length!=0;
    if(common.file){
        valid=valid&&split.open(file_source(common),resource,source.language==spire::BootLanguage::English?spire::ResourceLanguage::English:spire::ResourceLanguage::Chinese);
        if(valid)resource=split.source();
    }else{source.language=spire::boot_probe_language(resource);installed_languages=source.language==spire::BootLanguage::English?1u:2u;}
    if(!valid||!app.init(resource,{save_game,load_game,clear_save,seed_entropy,load_preferences,save_preferences,available_languages,request_language})){
        boot_error(blit,spire::BootText::InvalidResources,source.language);return 5;
    }
    if(selected_language&&source.language!=preferred)store_language(source.language);
    performance.pack_bytes=resource.length;
    if(performance.enabled){const uint32_t now=timer_ticks();performance.startup.add(startup_start-now);
        if(source.phase!=spire::ResourceStage::Count)performance.startup_phases[(unsigned)source.phase].add(source.phase_start-now);}
    source.blit=nullptr; // Startup progress cannot repaint over the live game.
    auto change_language=[&](){
        const unsigned requested=pending_language;pending_language=0;
        if(!requested||!common.file||!app.title)return;
        void*next=open_sibling(launcher,requested==1?"nSpire2.eng.tns":"nSpire2.zhs.tns");
        if(!next){app.message(app.content.english()?"Language resources are unavailable.":"无法读取语言资源。");return;}
        void*previous=source.file;const auto old_language=source.language;
        source.file=next;source.language=static_cast<spire::BootLanguage>(requested);
        const bool opened=split.open(file_source(common),file_source(source),static_cast<spire::ResourceLanguage>(requested));
        if(opened&&app.reload_language(split.source())){nd_close(previous);store_language(source.language);}
        else{
            nd_close(next);source.file=previous;source.language=old_language;
            if(!split.open(file_source(common),file_source(source),static_cast<spire::ResourceLanguage>(old_language))||!app.reload_language(split.source())){
                boot_error(blit,spire::BootText::InvalidResources,old_language);app.quit=true;return;
            }
            app.message(app.content.english()?"Could not change language.":"无法切换语言。");
        }
    };
    while(pressed(0x10,2)){} // Release ENTER used to open the document.
    Touchpad pad{};pad.init();spire::TouchpadCursor pad_cursor;
    clock.last=timer_ticks();uint32_t input_clock=0;
    const spire::Key keys[]={spire::Key::Left,spire::Key::Right,spire::Key::Up,spire::Key::Down,spire::Key::Confirm,spire::Key::Back,spire::Key::EndTurn,spire::Key::Inspect,spire::Key::Save,spire::Key::Load,spire::Key::Log,spire::Key::Map,spire::Key::Potions,spire::Key::Relics,spire::Key::Detail,spire::Key::ReferenceMap,spire::Key::Collection,spire::Key::Statuses,spire::Key::DrawPile,spire::Key::DiscardPile,spire::Key::ExhaustPile
#if SPIRE_DEBUG_COMMANDS
    ,spire::Key::Debug
#endif
    };
    uint32_t previous=0xffff;unsigned repeat[4]={};bool input_ready=false;unsigned quiet_ms=0;
    uint32_t first_start=performance.enabled?timer_ticks():0;
    app.render();uint32_t first_present=performance.enabled?timer_ticks():0;blit(app.pixels);
    uint32_t last_present=performance.enabled?timer_ticks():0;
    if(performance.enabled){performance.first_render.add(first_start-first_present);performance.first_present.add(first_present-last_present);}
    auto render_frame=[&](uint32_t update_start,const spire::RuntimeResourceTiming&before,spire::RuntimeScene update_scene){
        uint32_t render_start=performance.enabled?timer_ticks():0;
        auto scene=performance.enabled?performance_scene():spire::RuntimeScene::Other;
        app.render();uint32_t present_start=performance.enabled?timer_ticks():0;blit(app.pixels);
        if(performance.enabled){uint32_t done=timer_ticks();
            // The last preparation update may finish before render starts;
            // the first preparation screen may become active inside render.
            // Keep either transition's indivisible interval out of Combat.
            if(update_scene==spire::RuntimeScene::CombatPreparation||
               performance_scene()==spire::RuntimeScene::CombatPreparation)scene=spire::RuntimeScene::CombatPreparation;
            performance.frame(scene,last_present-done,update_start-render_start,render_start-present_start,present_start-done,before);
            last_present=done;}
    };
    while(!app.quit){
        unsigned elapsed=clock.frame();if(!elapsed)continue;
        uint32_t update_start=performance.enabled?timer_ticks():0;
        const auto update_scene=performance.enabled?performance_scene():spire::RuntimeScene::Other;
        const auto resources_before=performance.enabled?performance.resources:spire::RuntimeResourceTiming{};
        app.advance(elapsed);
        input_clock+=elapsed;
        auto sample=pad.read();auto pointer=pad_cursor.sample(sample,input_clock);
        if(input_ready&&pointer.cancelled)app.pointer_cancel();
        if(input_ready&&(pointer.moved||pointer.down||pointer.up||pointer.tap))app.pointer_move(pointer.x,pointer.y);
        if(input_ready&&(pointer.down||pointer.tap))app.pointer_button(true);
        if(input_ready&&(pointer.up||pointer.tap))app.pointer_button(false);
        uint32_t p=0;
        if(pressed(0x14,0x80))p|=1u<<0; // 4
        if(pressed(0x14,8))p|=1u<<1; // 6
        if(pressed(0x1C,0x40))p|=1u<<2; // 8
        if(pressed(0x1C,0x10))p|=1u<<3; // 2
        if(pressed(0x10,2)||pressed(0x1A,0x40))p|=1u<<4; // ENTER / 5
        if(pressed(0x1C,0x80))p|=1u<<5; // ESC
        if(pressed(0x10,0x10)||pressed(0x18,2))p|=1u<<6; // SPACE / E
        if(pressed(0x1C,0x200)||pressed(0x14,2))p|=1u<<7; // TAB / Q
        if(pressed(0x12,0x40))p|=1u<<8; // S
        if(pressed(0x14,0x10))p|=1u<<9; // O
        if(pressed(0x16,1))p|=1u<<10; // L
        if(pressed(0x14,0x40))p|=1u<<11; // M
        if(pressed(0x14,4))p|=1u<<12; // P
        if(pressed(0x14,1))p|=1u<<13; // R: relic collection
        if(pressed(0x16,0x10))p|=1u<<14; // I: inspect selected card
#if SPIRE_DEBUG_COMMANDS
        if(pressed(0x16,0x40))p|=1u<<21; // G: developer command menu
#endif
        if(pressed(0x18,0x10))p|=1u<<16; // C: compendium
        if(pressed(0x12,4))p|=1u<<17; // V: powers and statuses
        if(pressed(0x18,0x40))p|=1u<<18; // A: draw pile
        if(pressed(0x18,4))p|=1u<<19; // D: discard pile
        if(pressed(0x18,1))p|=1u<<20; // F: exhaust pile
        if(!input_ready){
            // The document launcher may leave ENTER or the pad held. Require a
            // stable release so opening the program cannot also start a run.
            quiet_ms=(p==0&&!sample.contact&&!sample.pressed)?quiet_ms+elapsed:0;
            if(quiet_ms>=100)input_ready=true;
            previous=p;pad_cursor.reset();render_frame(update_start,resources_before,update_scene);continue;
        }
        uint32_t edge=p&~previous;
        for(unsigned i=0;i<4;i++){if(p&(1u<<i)){repeat[i]++;if(repeat[i]>10&&repeat[i]%3==0)edge|=1u<<i;}else repeat[i]=0;}
        for(unsigned i=0;i<sizeof(keys)/sizeof(keys[0]);i++)if(edge&(1u<<i))app.input(keys[i]);previous=p;
        change_language();
        // Quit is already committed. A discarded final frame can perform
        // resource syscalls while ENTER is still held and expose that press
        // to the document browser when Ndless restores the foreground.
        if(!app.quit)render_frame(update_start,resources_before,update_scene);
    }
    auto held=[&](){
        for(unsigned offset=0x10;offset<0x20;offset+=2)if(pressed(offset,0xffffu))return true;
        const auto sample=pad.read();return sample.contact||sample.pressed;
    };
    exit_input_release(timer_ticks,held);
    // Finish filesystem work before the final release gate. In particular,
    // two split-resource destructors must not issue OS calls after it.
    source.close();common.close();performance_report();
    exit_input_release(timer_ticks,held);
    display.restore();return 0;
}
