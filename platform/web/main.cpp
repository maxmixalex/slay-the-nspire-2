#include "app.hpp"
#include "locale_boot.hpp"
#include "runtime_scene.hpp"
#if SPIRE_DEBUG_COMMANDS
#include "original_events.hpp"
#endif
extern "C" {
__attribute__((import_module("env"),import_name("save"))) int host_save(const uint8_t*,unsigned);
__attribute__((import_module("env"),import_name("load"))) int host_load(uint8_t*,unsigned);
__attribute__((import_module("env"),import_name("resource_read"))) int host_resource_read(unsigned,uint8_t*,unsigned);
}
static spire::App app;
static spire::SaveData buffer;
static spire::SplitResourceSource split;
static spire::BootLanguage boot_language=spire::BootLanguage::Unknown;
static uint32_t browser_entropy=0;
static spire::UiPreferences preferences{};
static bool preferences_dirty=false;
static unsigned language_mask=0,pending_language=0;
static bool load_preferences(spire::UiPreferences& out){out=preferences;return true;}
static bool save_preferences(const spire::UiPreferences& value){preferences=value;preferences_dirty=true;return true;}
static unsigned available_languages(){return language_mask?language_mask:(app.content.english()?1u:2u);}
static bool request_language(unsigned value){
    if(value<1||value>2||!(available_languages()&(1u<<(value-1))))return false;
    pending_language=value;return true;
}
static uint32_t seed_entropy(){return browser_entropy;}
static bool read_resource(void*,uint32_t offset,void* output,uint32_t count){return host_resource_read(offset,(uint8_t*)output,count)!=0;}
// Keep the original single resource_read import for existing replay hosts.
// Each physical split file fits below 256 MiB, so these disjoint host windows
// require no combined ArrayBuffer and no additional calculator/WASM memory.
static bool read_common(void*,uint32_t offset,void*output,uint32_t count){return host_resource_read(0x10000000u+offset,(uint8_t*)output,count)!=0;}
static bool read_language(void*,uint32_t offset,void*output,uint32_t count){return host_resource_read(0x20000000u+offset,(uint8_t*)output,count)!=0;}
static bool save_game(const spire::State&s){return spire::save(s,buffer)&&host_save(buffer.bytes,(unsigned)buffer.size)!=0;}
static bool load_game(spire::State&s){int n=host_load(buffer.bytes,sizeof buffer.bytes);return n>0&&spire::load(s,buffer.bytes,(unsigned)n);}
// The paired browser host treats an explicit empty save as deletion. Normal
// saves always contain the validated NSP save header and are nonempty.
static bool clear_save(){return host_save(nullptr,0)!=0;}
extern "C" {
// Optional host hook preserves the existing imports for deterministic replay.
// Interactive browsers set this from crypto before dispatching user input.
void set_seed_entropy(uint32_t value){browser_entropy=value;}
// Optional exports retain the established three-import ABI. Preferences are
// stored by the browser separately from the player's run, in either language.
void set_preferences(unsigned flags){preferences.fast_mode=flags==1;preferences_dirty=false;}
unsigned preferences_flags(){return preferences.fast_mode?1u:0u;}
void set_preferences_v2(unsigned flags){spire::UiPreferences value{};if(spire::ui_preferences_from_flags(flags,value))preferences=value;preferences_dirty=false;}
unsigned preferences_v2_flags(){return spire::ui_preference_flags(preferences);}
int preferences_pending(){return preferences_dirty?1:0;}
void preferences_ack(int success){
    preferences_dirty=false;
    if(!success)app.message(app.content.english()?"Could not save settings.":"设置保存失败，请重试");
}
int start(unsigned n){
    split.close();const spire::ResourceSource resource{nullptr,n,read_resource,nullptr};
    boot_language=spire::boot_probe_language(resource);
    pending_language=0;
    return app.init(resource,{save_game,load_game,clear_save,seed_entropy,load_preferences,save_preferences,available_languages,request_language})?1:0;
}
int start_split(unsigned common_bytes,unsigned language_bytes,unsigned language){
    split.close();if(language!=1&&language!=2)return 0;
    boot_language=language==1?spire::BootLanguage::English:spire::BootLanguage::Chinese;
    if(!split.open({nullptr,common_bytes,read_common,nullptr},
        {nullptr,language_bytes,read_language,nullptr},static_cast<spire::ResourceLanguage>(language)))return 0;
    pending_language=0;
    return app.init(split.source(),{save_game,load_game,clear_save,seed_entropy,load_preferences,save_preferences,available_languages,request_language})?1:0;
}
void set_available_languages(unsigned mask){language_mask=mask&3u;}
unsigned language_request(){return pending_language;}
int switch_language_split(unsigned common_bytes,unsigned language_bytes,unsigned language){
    pending_language=0;
    if(!app.title||language<1||language>2||!(available_languages()&(1u<<(language-1))))return 0;
    if(!split.open({nullptr,common_bytes,read_common,nullptr},
        {nullptr,language_bytes,read_language,nullptr},static_cast<spire::ResourceLanguage>(language)))return 0;
    if(!app.reload_language(split.source()))return 0;
    boot_language=language==1?spire::BootLanguage::English:spire::BootLanguage::Chinese;return 1;
}
int resource_language(){return app.content.english()?1:2;}
unsigned framebuffer(){return (unsigned)(size_t)app.pixels;}
void input(int key){if(key>=0&&key<=(int)spire::Key::Debug)app.input((spire::Key)key);}
void frame(){app.tick();app.render();}
void advance(unsigned elapsed_ms){app.advance(elapsed_ms);}
void draw_frame(){app.render();}
void pointer_move(int x,int y){app.pointer_move(x,y);}
void pointer_button(int down){app.pointer_button(down!=0);}
void pointer_cancel(){app.pointer_cancel();app.pointer.visible=false;}
void pointer_scroll(int rows){app.pointer_scroll(rows);}
int text_entry_active(){
#if SPIRE_DEBUG_COMMANDS
    if(app.debug_open)return app.debug_editing;
#endif
    return app.map_preview.open&&app.map_preview.editing;
}
void text_input(int ch){
#if SPIRE_DEBUG_COMMANDS
    if(app.debug_open){app.debug_text(ch);return;}
#endif
    app.reference_text(ch);
}
#if SPIRE_DEBUG_COMMANDS
unsigned debug_buffer(){return (unsigned)(size_t)app.debug_line;}
unsigned debug_capacity(){return sizeof(app.debug_line);}
int debug_execute(){return app.debug_command(app.debug_line)?1:0;}
unsigned debug_result(){return (unsigned)(size_t)app.debug_reply;}
unsigned debug_catalog_count(unsigned kind){return kind==1?spire::EncounterKinds:kind==2?unsigned(spire::OriginalEventId::Count):0;}
unsigned debug_catalog(unsigned kind,unsigned index){
    const char*key="";
    if(kind==1&&index<unsigned(spire::EncounterKinds)){const auto*d=spire::encounter_definition(spire::EncounterId(index));if(d)key=d->key;}
    if(kind==2&&index<unsigned(spire::OriginalEventId::Count))key=spire::original_event_key(spire::OriginalEventId(index));
    return (unsigned)(size_t)key;
}
#endif
int closed(){return app.quit?1:0;}
// Read-only scene classification for opt-in host timing; no gameplay mutation.
int performance_scene(){
    return int(spire::runtime_scene(app));
}
}
