#include "app.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstring>
using namespace spire;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static App app;
static UiPreferences persisted{};
static unsigned writes=0;
static bool read_ok=true,write_ok=true;
static bool read_settings(UiPreferences&out){out=persisted;return read_ok;}
static bool write_settings(const UiPreferences&value){++writes;if(!write_ok)return false;persisted=value;return true;}
int main(){
    uint8_t encoded[UiPreferencesBytes+1]{};UiPreferences result{};
    encode_ui_preferences({true},encoded);CHECK(decode_ui_preferences(encoded,UiPreferencesBytes,result)&&result.fast_mode);
    CHECK(!decode_ui_preferences(encoded,UiPreferencesBytes+1,result));
    for(unsigned i=0;i<UiPreferencesBytes;++i){auto old=encoded[i];encoded[i]=255;CHECK(!decode_ui_preferences(encoded,UiPreferencesBytes,result));encoded[i]=old;}
    UiPreferences timer{};timer.show_run_timer=true;timer.bindings.keys[0][unsigned(InputCommand::Map)]='Y';
    encode_ui_preferences(timer,encoded);CHECK(encoded[4]==4&&UiPreferencesBytes==108);
    CHECK(decode_ui_preferences(encoded,UiPreferencesBytes,result)&&result.show_run_timer&&result.bindings.keys[0][unsigned(InputCommand::Map)]=='Y');
    encoded[4]=3;encoded[5]&=~64;CHECK(decode_ui_preferences(encoded,UiPreferencesBytes,result)&&!result.show_run_timer&&result.bindings.keys[0][unsigned(InputCommand::Map)]=='Y');
    encoded[5]|=64;CHECK(!decode_ui_preferences(encoded,UiPreferencesBytes,result)); // v3 never assigned this bit.
    app.callbacks.load_preferences=read_settings;app.callbacks.save_preferences=write_settings;
    app.restore_preferences();CHECK(!app.fast_mode);
    app.set_fast_mode(true);CHECK(writes==1&&persisted.fast_mode);
    app.set_fast_mode(true);CHECK(writes==1);
    app.fast_mode=false;app.restore_preferences();CHECK(app.fast_mode); // restart / both language hosts
    read_ok=false;app.restore_preferences();CHECK(!app.fast_mode); // failed callback cannot leak a partial read
    write_ok=false;app.set_fast_mode(true);CHECK(writes==2&&app.notice_ticks>0);
    // This preference is neither a run option nor part of any historical save.
    new_run(app.game,123,true);SaveData before{},after{};CHECK(save(app.game,before));
    app.set_fast_mode(false);CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    // The real timer reads observed play time and never changes the game save.
    read_ok=write_ok=true;new_run(app.game,7319,false);CHECK(world_begin_run(app.game));
    app.title=app.paused=app.settings_open=false;app.show_run_timer=false;app.reset_presentation();app.begin_progress();
    char time[32]{};app.run_timer_label(time,sizeof time);CHECK(!std::strcmp(time,"00:00")&&!app.run_timer_visible());
    app.progress_time(1000);app.run_timer_label(time,sizeof time);CHECK(!std::strcmp(time,"00:01"));
    app.paused=true;CHECK(app.run_timer_visible());app.progress_time(2000);app.paused=false;app.run_timer_label(time,sizeof time);CHECK(!std::strcmp(time,"00:01"));
    app.open_map_window(true);CHECK(app.run_timer_visible());app.close_map_window();CHECK(app.run_timer_visible());
    app.map_window.update(app.map_clock_ms+world_map_window::CloseDuration);CHECK(!app.run_timer_visible());
    app.panel=1;CHECK(app.run_timer_visible());app.panel=0;CHECK(!app.run_timer_visible());
    app.panel=4;CHECK(!app.run_timer_visible());app.panel=0; // A potion context menu is not a capstone.
    CHECK(save(app.game,before));app.settings_open=true;app.ui_action(UiAction::SettingsRunTimer);CHECK(app.show_run_timer&&persisted.show_run_timer);
    app.progress_time(2000);app.run_timer_label(time,sizeof time);CHECK(!std::strcmp(time,"00:01"));app.settings_open=false;
    app.show_run_timer=false;app.restore_preferences();CHECK(app.show_run_timer&&app.run_timer_visible());
    app.progress.current.seconds=3723;app.run_timer_label(time,sizeof time);CHECK(!std::strcmp(time,"1:02:03"));
    app.progress.current.partial=1;app.run_timer_label(time,sizeof time);CHECK(!std::strcmp(time,"—"));
    CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    app.title=true;CHECK(!app.run_timer_visible());
    return 0;
}
