#include "calculator_pad_input.hpp"
#include "app.hpp"
#include "host_text_input.hpp"
#include "host_physical_input.hpp"
#include "host_input_latch.hpp"
#include "departure_frame_clock.hpp"
#include "resource_file_cursor.hpp"
#include "runtime_performance.hpp"
#include "runtime_scene.hpp"
#include "touchpad.hpp"
#include "loading_screen.hpp"
#include "resource_transfer.hpp"
#include "installation_paths.hpp"
#include "calculator_system_language.hpp"
#include "lzma2_block.hpp"
extern "C" {
void* nd_open(const char*,const char*);
unsigned nd_read(void*,unsigned,unsigned,void*);
unsigned nd_write(const void*,unsigned,unsigned,void*);
int nd_close(void*);
int nd_seek(void*,long,int);
long nd_tell(void*);
int nd_rename(const char*,const char*);
int nd_unlink(const char*);
int nd_mkdir(const char*);
void* nd_opendir(const char*);
int nd_closedir(void*);
unsigned nd_hwtype();
unsigned nd_revision();
int nd_lcd_init(int);
void (*nd_blitter(int))(void*);
int nd_inflate_init(void*,int,const char*,int);
int nd_inflate(void*,int);
int nd_inflate_end(void*);
int nd_touch_read(unsigned,unsigned,void*);
int nd_touch_write(unsigned,unsigned,const void*);
int nd_has_syscall(unsigned);
void* nd_calc_address();
int nd_math_to_string(void*,void*,uint16_t**);
void nd_os_free(void*);
}
static spire::App app;
static spire::RuntimePerformance performance;
static char pack_path[512],save_path[512],temp_path[512],backup_path[512],performance_path[512],profile_path[512],language_path[512];
static char settings_path[512],settings_temp[512],settings_backup[512];
static char progress_path[512],progress_temp[512],progress_backup[512];
static_assert(spire::UiProgressCapacity+1<=spire::ResourceTransferCapacity,"History transfers fit the shared foreground workspace");
static uint32_t wall_clock(){return *(volatile uint32_t*)0x90090000u;}
static uint32_t timer_ticks(){return *(volatile uint32_t*)0x900D0004u;}
static uint32_t foreground_clock_ticks(){return 0u-timer_ticks();}
static void performance_render_part(spire::RuntimeRenderPart part,bool begin){
    performance.render_part(part,begin,timer_ticks());
}
static uint32_t entropy_start=0;
static unsigned installed_languages=0,pending_language=0;
static spire::CalculatorInputLatch input_latch;
struct Touchpad;
static Touchpad*resource_pad=nullptr;
static spire::CalculatorPadInput resource_pad_capture;
static spire::CalculatorForegroundPoll foreground_input_poll;
static void read_key_matrix(uint16_t*matrix){
    for(unsigned row=0;row<8;++row)matrix[row]=*(volatile uint16_t*)(0x900E0010u+row*2);
}
static void capture_key_matrix();
static int sampled_seek(void*file,long offset,int origin){
    capture_key_matrix();const int result=nd_seek(file,offset,origin);capture_key_matrix();return result;
}
static unsigned sampled_read(void*output,unsigned size,unsigned count,void*file){
    if(!input_latch.enabled||size!=1)return nd_read(output,size,count,file);
    return spire::sampled_resource_read(output,count,file,nd_read,capture_key_matrix);
}
static unsigned available_languages(){return installed_languages;}
static bool request_language(unsigned language){
    if(!spire::locale_language_valid(language)||!(installed_languages&spire::locale_language_mask(language)))return false;
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
    bool good=source.cursor.read(source.file,offset,output,count,sampled_seek,sampled_read,seek);
    if(performance.enabled)performance.read(started-timer_ticks(),count,good,seek);
    return good;
}
static bool state_sibling(char*out,const char*launcher,const char*filename){
    return spire::installation_path(out,512,launcher,spire::InstallationDirectory::State,filename);
}
static bool ensure_state_directory(const char*launcher){
    char directory[512];
    if(!spire::installation_path(directory,sizeof directory,launcher,spire::InstallationDirectory::Root,"state"))return false;
    if(void*existing=nd_opendir(directory))return nd_closedir(existing)==0;
    return nd_mkdir(directory)==0;
}
#include "profile_storage.inc"
static void* open_data_file(const char*launcher,const char*name){return spire::installation_path(pack_path,sizeof pack_path,launcher,spire::InstallationDirectory::Data,name)?nd_open(pack_path,"rb"):nullptr;}
static spire::ResourceSource file_source(ResourceFile&file){
    file.cursor={}; // This length probe moves the underlying stream as well.
    if(!file.file||nd_seek(file.file,0,2)!=0)return {};
    const long length=nd_tell(file.file);
    if(length<=0||(unsigned long)length>spire::MaxContentFile)return {};
    return {&file,(uint32_t)length,read_resource,nullptr,resource_progress};
}
#include "preferences_storage.inc"
#include "progress_storage.inc"
#include "run_storage.inc"
static bool clear_save(){
    // A deliberate abandon also removes the preceding atomic-save backup.
    // Otherwise a later recovery could bring back an abandoned expedition.
    const char*paths[]={backup_path,temp_path,save_path};for(unsigned i=0;i<3;i++){const char*path=paths[i];
        void*f=nd_open(path,"rb");if(!f)continue;if(nd_close(f)!=0||nd_unlink(path)!=0)return false;
    }return true;
}
#include "feedback_storage.inc"
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
static unsigned system_language(){
    // These are the official Ndless SDK calc_cmd/MathExprToStr bindings. The
    // former returns the OS function address, preserving the fifth argument's
    // normal ARM ABI instead of shifting it with a four-argument SWI stub.
    if(nd_has_syscall(339)&&nd_has_syscall(342)){
        const unsigned language=spire::calculator_system_language(
            reinterpret_cast<spire::CalculatorLocaleEvaluate>(nd_calc_address()),
            nd_math_to_string,nd_os_free);
        if(language)return language;
    }
    // OS3.1 does not export calc_cmd/MathExprToStr, but stores the same actual
    // ISO tag in its settings archive. Reuse the foreground transfer memory
    // and OS inflater; neither a resident ZIP engine nor another pool is added.
    spire::ResourceTransferLease lease(spire::ResourceTransferUse::Bytes);
    if(!lease)return 0;
    void*file=nd_open("/phoenix/syst/settings/current.zip","rb");if(!file)return 0;
    uint8_t*bytes=lease.bytes();const unsigned count=nd_read(bytes,1,16384,file);nd_close(file);
    spire::CalculatorSettingsEntry entry;
    if(!spire::calculator_settings_entry(bytes,count,entry))return 0;
    if(!entry.deflated)return spire::calculator_language_xml(entry.data,entry.plain);
    struct Stream {
        const uint8_t*next_in;uint32_t avail_in,total_in;
        uint8_t*next_out;uint32_t avail_out,total_out;
        void*message;void*state;void*allocate;void*release;void*opaque;
        int32_t data_type;uint32_t adler,reserved;
    } stream{};
    static_assert(sizeof(Stream)==56,"ARM32 zlib stream ABI");
    stream.next_in=entry.data;stream.avail_in=entry.compressed;
    stream.next_out=bytes+16384;stream.avail_out=entry.plain;
    if(nd_inflate_init(&stream,-15,"1.2.3",sizeof stream)!=0)return 0;
    const int decoded=nd_inflate(&stream,4);nd_inflate_end(&stream);
    return decoded==1&&stream.total_out==entry.plain&&stream.avail_in==0?
        spire::calculator_language_xml(bytes+16384,entry.plain):0;
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
static void capture_key_matrix(){
    if(!foreground_input_poll.due(timer_ticks(),input_latch.enabled))return;
    uint16_t matrix[8];read_key_matrix(matrix);
    // Filesystem, decoder and coarse rendering boundaries all collect into
    // one FIFO. No IRQ, App input dispatch or animation update occurs here.
    const auto touch=resource_pad?resource_pad->read():spire::TouchSample{};
    resource_pad_capture.capture(touch,app.keyboard_only);
    foreground_input_poll.mark(timer_ticks());
    input_latch.sample(matrix,resource_pad_capture.captured,touch,foreground_input_poll.milliseconds);
}
extern "C" int main(int argc,char**argv){
    if(nd_revision()<2022||nd_hwtype()==0)return 2;
    const char*launcher=argc>0&&argv&&argv[0]?argv[0]:"";
    profile_launcher=launcher;
    if(!ensure_state_directory(launcher))return 3;
    if(!state_sibling(performance_path,launcher,"nSpire2.perf.tns")||!state_sibling(profile_path,launcher,"nSpire2.profile.tns")||!state_sibling(language_path,launcher,"nSpire2.lang.tns")||!state_sibling(selector_path,launcher,"nSpire2.slots.tns")||!state_sibling(selector_temp,launcher,"nSpire2.slots.tmp.tns")||!state_sibling(selector_backup,launcher,"nSpire2.slots.bak.tns"))return 3;
    selected_profile=load_profile_selector();if(!configure_profile_paths(selected_profile))return 3;
    // Ndless may wait for all launch keys to be released before entering main,
    // so a held CTRL alone cannot reliably opt in. An owner-created state/profile
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
    common.file=open_data_file(launcher,spire::SharedDataFilename);
    if(!common.file){boot_error(blit,spire::BootText::MissingResources,spire::BootLanguage::Unknown);return 4;}
    if(common.file){
        // Probe installed optional language files; retain only the chosen handle.
        // A language change reloads metadata/fonts while artwork stays shared.
        for(unsigned id=1;id<=spire::LocaleLanguageCount;++id){
            void*f=open_data_file(launcher,spire::locale_language_info(id).file);
            if(f){installed_languages|=spire::locale_language_mask(id);nd_close(f);}
        }
        if(!installed_languages){boot_error(blit,spire::BootText::MissingLanguage,spire::BootLanguage::Unknown);return 4;}
        const unsigned saved=unsigned(preferred);
        const unsigned system=(installed_languages&spire::locale_language_mask(saved))?0:system_language();
        const unsigned selected=spire::locale_default_language(installed_languages,saved,system);
        source.language=static_cast<spire::BootLanguage>(selected);
        source.file=open_data_file(launcher,spire::locale_language_info(selected).file);
        selected_language=true;
    }
    if(!source.file){boot_error(blit,spire::BootText::MissingResources,spire::BootLanguage::Unknown);return 4;}
    const uint32_t startup_start=performance.enabled?timer_ticks():0;
    source.blit=blit;auto resource=file_source(source);bool valid=resource.length!=0;
    if(common.file){
        valid=valid&&split.open(file_source(common),resource,static_cast<spire::ResourceLanguage>(source.language));
        if(valid)resource=split.source();
    }
    app.calculator_keyboard=true;
    if(!valid||!app.init(resource,{save_game,load_game,clear_save,seed_entropy,load_preferences,save_preferences,available_languages,request_language,load_progress,save_progress,wall_clock,current_profile,profile_info,request_profile,write_feedback})){
        boot_error(blit,spire::BootText::InvalidResources,source.language);return 5;
    }
    if(performance.enabled)app.callbacks.profile_render=performance_render_part;
    app.callbacks.foreground_clock_ticks=foreground_clock_ticks;
    app.callbacks.foreground_poll=capture_key_matrix;
    spire::set_lzma2_foreground_poll(capture_key_matrix);
    if(selected_language&&source.language!=preferred)store_language(source.language);
    performance.pack_bytes=resource.length;
    if(performance.enabled){const uint32_t now=timer_ticks();performance.startup.add(startup_start-now);
        if(source.phase!=spire::ResourceStage::Count)performance.startup_phases[(unsigned)source.phase].add(source.phase_start-now);}
    source.blit=nullptr; // Startup progress cannot repaint over the live game.
    auto change_language=[&](){
        const unsigned requested=pending_language;pending_language=0;
        if(!requested||!common.file||!app.title)return;
        void*next=open_data_file(launcher,spire::locale_language_info(requested).file);
        if(!next){app.message(app.content.language()!=2?"Language resources are unavailable.":"无法读取语言资源。");return;}
        void*previous=source.file;const auto old_language=source.language;
        source.file=next;source.language=static_cast<spire::BootLanguage>(requested);
        const bool opened=split.open(file_source(common),file_source(source),static_cast<spire::ResourceLanguage>(requested));
        if(opened&&app.reload_language(split.source())){nd_close(previous);store_language(source.language);}
        else{
            nd_close(next);source.file=previous;source.language=old_language;
            if(!split.open(file_source(common),file_source(source),static_cast<spire::ResourceLanguage>(old_language))||!app.reload_language(split.source())){
                boot_error(blit,spire::BootText::InvalidResources,old_language);app.quit=true;return;
            }
            app.message(app.content.language()!=2?"Could not change language.":"无法切换语言。");
        }
    };
    while(pressed(0x10,2)){} // Release ENTER used to open the document.
    app.begin_startup_logo();
    Touchpad pad{};pad.init();resource_pad=&pad;spire::TouchpadCursor pad_cursor;spire::CalculatorPadInput pad_keys;
    clock.last=timer_ticks();uint32_t input_clock=0;
    uint16_t text_previous[8]{};uint8_t repeat[spire::CalculatorPhysicalKeyCount]{};bool input_ready=false;unsigned quiet_ms=0;
    uint32_t first_start=performance.enabled?timer_ticks():0;
    app.render();uint32_t first_present=performance.enabled?timer_ticks():0;blit(app.pixels);
    uint32_t last_present=performance.enabled?timer_ticks():0;
    spire::DepartureFrameClock departure_clock;
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
        if(departure_clock.presented(app.character_departure.phase,app.combat_preparation.active))
            clock.last=timer_ticks();
    };
    while(!app.quit){
        unsigned elapsed=clock.frame();if(!elapsed)continue;
        uint32_t update_start=performance.enabled?timer_ticks():0;
        const auto update_scene=performance.enabled?performance_scene():spire::RuntimeScene::Other;
        const auto resources_before=performance.enabled?performance.resources:spire::RuntimeResourceTiming{};
        app.advance(elapsed);
        auto sample=pad.read();
        resource_pad_capture.capture(sample,app.keyboard_only);
        uint16_t text_keys[8];read_key_matrix(text_keys);
        foreground_input_poll.mark(timer_ticks());
        input_clock=foreground_input_poll.milliseconds;
        if(!input_ready){
            // The document launcher may leave ENTER or the pad held. Require a
            // stable release so opening the program cannot also start a run.
            bool held_key=false;spire::calculator_physical_keys([&](unsigned,unsigned row,unsigned mask,uint16_t){if(text_keys[row]&mask)held_key=true;});
            quiet_ms=(!held_key&&!sample.contact&&!sample.pressed)?quiet_ms+elapsed:0;
            if(quiet_ms>=100)input_ready=true;
            input_latch.reset(text_keys,input_ready,resource_pad_capture.captured,sample);
            for(unsigned row=0;row<8;++row)text_previous[row]=text_keys[row];pad_cursor.reset();
            if(input_ready)pad_cursor.sample(sample,input_clock);
            render_frame(update_start,resources_before,update_scene);continue;
        }
        input_latch.sample(text_keys,resource_pad_capture.captured,sample,input_clock);
        const auto dispatch_matrix=[&](bool allow_repeat){
        const bool library_text=app.library&&!app.library_tab&&app.library_search_editing&&app.binding_listening<0;
        const bool seed_text=app.map_preview.open&&app.map_preview.editing&&app.binding_listening<0;
        bool console_text=false;
        console_text=app.debug_open;
        if((text_keys[6]&1)&&!(text_previous[6]&1))app.input(spire::Key::Debug);
        console_text=app.debug_open;
        spire::calculator_physical_edges(text_keys,text_previous,repeat,library_text||seed_text||console_text,
            [](uint16_t code,bool down){if(code!=spire::InputFlag)app.physical_key(code,down);},allow_repeat);
        if(console_text){spire::calculator_text_edges(text_keys,text_previous,[](int code){app.debug_text(code);});
            if((text_keys[6]&0x200)&&!(text_previous[6]&0x200))app.debug_text(9);}
        else
        if(library_text)spire::calculator_text_edges(text_keys,text_previous,[](int code){app.library_text(code);});
        else if(seed_text)spire::calculator_text_edges(text_keys,text_previous,[](int code){app.reference_text(code);});
        for(unsigned row=0;row<8;++row)text_previous[row]=text_keys[row];
        };
        // Only drain changes captured before dispatch. An action can itself
        // read resources; input captured there belongs to the next update.
        const unsigned pending=input_latch.count;
        for(unsigned i=0;i<pending;++i){uint16_t pad_key=0;spire::TouchSample touch;uint32_t captured_at=0;
            input_latch.pop(text_keys,&pad_key,&touch,&captured_at);
            spire::dispatch_calculator_sample(pad_keys,pad_cursor,touch,pad_key,captured_at,app,false,dispatch_matrix);}
        if(!pending)spire::dispatch_calculator_sample(pad_keys,pad_cursor,sample,resource_pad_capture.captured,input_clock,app,true,dispatch_matrix);
        change_language();service_profile();
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
