#include "app.hpp"
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
    return 0;
}
