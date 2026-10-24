#include "app.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstring>
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
int main(){
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
    std::puts("profiles: three independent runs/progress, confirmation, failed switch and active deletion passed");
}
