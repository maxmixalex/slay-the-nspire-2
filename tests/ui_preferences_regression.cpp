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
    CHECK(UiPreferences{}.show_run_timer&&app.show_run_timer);
    uint8_t encoded[UiPreferencesBytes+1]{};UiPreferences result{};
    encode_ui_preferences({true},encoded);CHECK(decode_ui_preferences(encoded,UiPreferencesBytes,result)&&result.fast_mode);
    CHECK(!decode_ui_preferences(encoded,UiPreferencesBytes+1,result));
    for(unsigned i=0;i<UiPreferencesBytes;++i){auto old=encoded[i];encoded[i]=255;
        if(i==5)CHECK(decode_ui_preferences(encoded,UiPreferencesBytes,result)&&result.long_press&&result.show_run_timer);
        else CHECK(!decode_ui_preferences(encoded,UiPreferencesBytes,result));encoded[i]=old;}
    // All eight v5 flag bits are assigned; old records must still reject the
    // formerly reserved bit instead of silently enabling a new preference.
    for(unsigned version=1;version<=4;++version){encode_ui_preferences({},encoded);encoded[4]=uint8_t(version);encoded[5]=128;
        CHECK(!decode_ui_preferences(encoded,version<=2?8:UiPreferencesBytes,result));}
    // The skip-logo bit belongs to v6; it cannot reinterpret a reserved bit
    // in any previous record, including the two eight-byte formats.
    for(unsigned version=1;version<=5;++version){encode_ui_preferences({},encoded);encoded[4]=uint8_t(version);encoded[6]|=8;
        CHECK(!decode_ui_preferences(encoded,version<=2?8:UiPreferencesBytes,result));}
    encode_ui_preferences({},encoded);encoded[4]=7;CHECK(!decode_ui_preferences(encoded,UiPreferencesBytes,result));
    UiPreferences timer{};timer.show_run_timer=true;timer.long_press=true;timer.bindings.keys[0][unsigned(InputCommand::Map)]='Y';
    encode_ui_preferences(timer,encoded);CHECK(encoded[4]==6&&UiPreferencesBytes==108);
    CHECK(decode_ui_preferences(encoded,UiPreferencesBytes,result)&&result.show_run_timer&&result.long_press&&result.bindings.keys[0][unsigned(InputCommand::Map)]=='Y');
    UiPreferences hidden=timer;hidden.show_run_timer=false;encode_ui_preferences(hidden,encoded);
    CHECK(decode_ui_preferences(encoded,UiPreferencesBytes,result)&&!result.show_run_timer);encode_ui_preferences(timer,encoded);
    encoded[4]=5;CHECK(decode_ui_preferences(encoded,UiPreferencesBytes,result)&&result.long_press&&!result.skip_intro_logo);
    encoded[4]=4;encoded[5]&=~128;CHECK(decode_ui_preferences(encoded,UiPreferencesBytes,result)&&result.show_run_timer&&!result.long_press);
    encoded[4]=3;encoded[5]&=~64;CHECK(decode_ui_preferences(encoded,UiPreferencesBytes,result)&&!result.show_run_timer&&!result.long_press&&result.bindings.keys[0][unsigned(InputCommand::Map)]=='Y');
    encoded[5]|=64;CHECK(!decode_ui_preferences(encoded,UiPreferencesBytes,result)); // v3 never assigned this bit.
    for(unsigned shake=0;shake<=4;++shake)for(unsigned skip=0;skip<2;++skip){
        timer.screen_shake=uint8_t(shake);timer.skip_intro_logo=skip;encode_ui_preferences(timer,encoded);
        CHECK(decode_ui_preferences(encoded,UiPreferencesBytes,result)&&result.screen_shake==shake&&result.skip_intro_logo==skip&&result.long_press&&result.show_run_timer);
        CHECK(result.bindings.keys[0][unsigned(InputCommand::Map)]=='Y');
    }
    for(unsigned shake=5;shake<=7;++shake){encode_ui_preferences(timer,encoded);encoded[6]=uint8_t(8+shake);CHECK(!decode_ui_preferences(encoded,UiPreferencesBytes,result));}
    app.callbacks.load_preferences=read_settings;app.callbacks.save_preferences=write_settings;
    app.restore_preferences();CHECK(!app.fast_mode);
    app.set_fast_mode(true);CHECK(writes==1&&persisted.fast_mode);
    app.set_fast_mode(true);CHECK(writes==1);
    app.fast_mode=false;app.restore_preferences();CHECK(app.fast_mode); // restart / both language hosts
    read_ok=false;app.restore_preferences();CHECK(!app.fast_mode); // failed callback cannot leak a partial read
    write_ok=false;app.set_fast_mode(true);CHECK(writes==2&&app.notice_ticks>0);
    // This preference is neither a run option nor part of any historical save.
    read_ok=write_ok=true;new_run(app.game,123,true);SaveData before{},after{};CHECK(save(app.game,before));
    app.set_fast_mode(false);app.settings_open=true;app.ui_action(UiAction::SettingsLongPress);CHECK(app.long_press&&persisted.long_press);app.settings_open=false;
    app.long_press=false;app.restore_preferences();CHECK(app.long_press);CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    // Traverse the real general-settings rows, toggle and reload. Preference
    // changes remain independent of both current-run and historical bytes.
    app.title=true;app.settings_open=true;app.settings_tab=app.settings_item=app.settings_top=0;
    for(unsigned i=0;i<8;++i)app.input(Key::Down);CHECK(app.settings_item==8&&!app.skip_intro_logo);
    app.input(Key::Confirm);CHECK(app.skip_intro_logo&&persisted.skip_intro_logo&&app.notice_ticks>0);
    app.skip_intro_logo=false;app.restore_preferences();CHECK(app.skip_intro_logo);
    CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    app.settings_open=false;
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
    app.settings_open=true;app.ui_action(UiAction::SettingsReset);CHECK(app.frontend_confirm==4);app.frontend_choice=1;app.input(Key::Confirm);
    CHECK(!app.long_press&&!persisted.long_press&&!app.end_turn_hold.owner&&!app.skip_intro_logo&&!persisted.skip_intro_logo&&app.show_run_timer&&persisted.show_run_timer);
    return 0;
}
