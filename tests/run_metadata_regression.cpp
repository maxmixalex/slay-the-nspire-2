#include "app.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
using namespace spire;
static App app;
static UiProgress loaded;
static SaveData disk,canonical;
static uint8_t encoded[UiProgressCapacity];
static std::vector<uint8_t> history;
static uint32_t now=1792884600;
static bool run_ok=true,history_ok=true;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"run metadata:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static uint32_t wall(){return now;}
static bool store_run(const State&s){return run_ok&&save(s,disk);}
static bool read_run(State&s){return disk.size&&load(s,disk.bytes,disk.size);}
static bool store_history(const UiProgress&p){if(!history_ok)return false;const size_t n=encode_ui_progress(p,encoded,sizeof encoded);if(!n)return false;history.assign(encoded,encoded+n);return true;}
static bool read_history(UiProgress&p){return !history.empty()&&decode_ui_progress(history.data(),history.size(),p);}
static std::vector<uint8_t> encode(const UiProgress&p){const size_t n=encode_ui_progress(p,encoded,sizeof encoded);CHECK(n);return {encoded,encoded+n};}
static void start(uint32_t seed,bool resumed=false){new_run(app.game,seed,false);CHECK(world_begin_run(app.game,reference::ActKind::Overgrowth,0));app.title=false;app.begin_progress(resumed);}
int main(){
    app.callbacks.save=store_run;app.callbacks.load=read_run;app.callbacks.save_progress=store_history;
    app.callbacks.load_progress=read_history;app.callbacks.wall_clock=wall;
    start(1041);CHECK(!std::strcmp(app.progress.current.build_id,UiCurrentBuildId));CHECK(!app.progress.current_saved_at);
    CHECK(save(app.game,canonical));CHECK(app.save_current_run());CHECK(app.saved_at==now&&app.progress.current_saved_at==now);
    CHECK(disk.size==canonical.size&&!std::memcmp(disk.bytes,canonical.bytes,disk.size));
    CHECK(history[3]=='9'&&read_history(loaded)&&loaded.current_saved_at==now);
    CHECK(!std::strcmp(loaded.current.build_id,UiCurrentBuildId));
    const auto first=history;const uint32_t actual_saved=now;
    now+=600;app.progress_dirty=true;CHECK(app.persist_progress());CHECK(app.progress.last_saved_at==now);
    CHECK(app.refresh_save()&&app.saved_at==actual_saved); // Settings/history writes are not run saves.
    run_ok=false;CHECK(!app.save_current_run());CHECK(app.saved_at==actual_saved&&app.progress.current_saved_at==actual_saved);run_ok=true;
    // Identical run bytes carry the same canonical revision. If optional
    // metadata cannot persist, a restart can only know the prior observed
    // timestamp. The ordinary save-and-quit path remains blocked by false.
    history_ok=false;CHECK(!app.save_current_run()&&app.saved_at==now);
    const auto repeated=app.progress;CHECK(read_history(loaded));app.progress=loaded;
    CHECK(app.refresh_save()&&app.saved_at==actual_saved);app.progress=repeated;
    history_ok=true;CHECK(app.persist_progress());now+=60;
    ++app.game.revision;--app.game.hp;history_ok=false;CHECK(!app.save_current_run());
    CHECK(app.saved_at==now&&app.progress.current_saved_at==now&&app.progress_dirty); // Canonical write did succeed.
    CHECK(read_history(loaded));const auto pending=app.progress;app.progress=loaded;
    CHECK(app.refresh_save()&&!app.saved_at); // An older metadata revision cannot label a newer file.
    app.progress=pending;history_ok=true;CHECK(app.persist_progress());CHECK(app.refresh_save()&&app.saved_at==now);
    CHECK(app.load_saved_run()&&app.progress.current_saved_at==now);
    const uint32_t saved_revision=app.progress.current_saved_revision;
    ++app.game.revision;CHECK(store_run(app.game));CHECK(app.refresh_save()&&!app.saved_at);
    app.game.revision=saved_revision;CHECK(app.save_current_run());
    app.finish_progress(true);CHECK(app.history_at(0)&&!std::strcmp(app.history_at(0)->build_id,UiCurrentBuildId));
    start(1042,true);CHECK(!app.progress.current.build_id[0]&&!app.progress.current_saved_at);
    CHECK(app.save_current_run()&&app.saved_at==now);CHECK(!app.progress.current.build_id[0]); // No invented original build.

    // An untouched older profile remains byte-identical and explicitly unknown.
    clear_ui_progress(loaded);loaded.current.serial=1;loaded.tutorial_flags=FtueFresh;
    const auto eight=encode(loaded);CHECK(eight[3]=='8');
    loaded.current_saved_at=now;loaded.current_saved_revision=7;
    std::memcpy(loaded.current.build_id,UiCurrentBuildId,sizeof UiCurrentBuildId);
    const auto nine=encode(loaded);CHECK(nine[3]=='9'&&nine.size()==eight.size()+UiProgressMetadataBytes);
    CHECK(!std::memcmp(nine.data()+8,eight.data()+8,eight.size()-8));
    CHECK(decode_ui_progress(nine.data(),nine.size(),loaded)&&encode(loaded)==nine);
    UiProgressSummary summary{};CHECK(decode_ui_progress_summary(nine.data(),nine.size(),summary)&&summary.exists);
    CHECK(!decode_ui_progress(nine.data(),nine.size()-1,loaded));
    auto bad=nine;bad[eight.size()+8]=1;CHECK(!decode_ui_progress(bad.data(),bad.size(),loaded));
    bad=nine;for(unsigned i=0;i<UiBuildIdBytes;++i)bad[eight.size()+8+i]='x';CHECK(!decode_ui_progress(bad.data(),bad.size(),loaded));
    bad=nine;bad[eight.size()+8+UiBuildIdBytes]='x';CHECK(!decode_ui_progress(bad.data(),bad.size(),loaded));
    CHECK(decode_ui_progress(eight.data(),eight.size(),loaded));CHECK(!loaded.current_saved_at&&!loaded.current.build_id[0]&&encode(loaded)==eight);

    const auto epoch=ui_calendar(0);CHECK(epoch.year==1970&&epoch.month==1&&epoch.day==1&&epoch.weekday==4&&!epoch.hour&&!epoch.minute);
    const auto leap=ui_calendar(951825600);CHECK(leap.year==2000&&leap.month==2&&leap.day==29&&leap.weekday==2&&leap.hour==12);
    const auto century=ui_calendar(4107542400u);CHECK(century.year==2100&&century.month==3&&century.day==1);
    const auto limit=ui_calendar(UINT32_MAX);CHECK(limit.year==2106&&limit.month==2&&limit.day==7&&limit.hour==6&&limit.minute==28);
    std::printf("Run metadata: %u checks passed; canonical v17 unchanged, NSH1-9, actual save/revision binding, unknown legacy build and calendar boundaries.\n",checks);
}
